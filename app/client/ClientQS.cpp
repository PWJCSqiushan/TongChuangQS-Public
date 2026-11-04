#include "ClientQS.h"

#include "DateQS.h"
#include "PathQS.h"

#include <algorithm>

namespace qs {

ClientQS::ClientQS(std::string dataRootUtf8)
    : platform_(std::make_unique<PlatformQS>(dataRootUtf8)), chat_(std::make_unique<ChatNodeQS>(dataRootUtf8)) {}

ClientQS::~ClientQS() {
    chat_->stop();
}

ResultQS ClientQS::start(bool allowLan) {
    const auto loaded = platform_->start();
    if (!loaded) return loaded;
    chat_->setValidator([this](const AccountRefQS& owner, const ChatMessageQS& m) { return acceptIncoming(owner, m); });
    return chat_->start(allowLan);
}

bool ClientQS::acceptIncoming(const AccountRefQS& owner, const ChatMessageQS& m) const {
    ServiceBaseQS* s = platform_->service(owner.service());
    if (s == nullptr || !s->supportsChat()) return false;
    s->reloadIfChanged();
    if (m.isGroup()) {
        const GroupCapabilityQS* groups = s->groupCapability();
        GroupQS g;
        return groups != nullptr && groups->findGroup(m.groupId, g) && g.hasMember(owner.account()) && g.hasMember(m.from);
    }
    const FriendCapabilityQS* friends = s->friendCapability();
    return friends != nullptr && friends->areFriends(owner.account(), m.from);
}

ResultQS ClientQS::bringChatOnline(const std::string& service) {
    ServiceBaseQS* s = platform_->service(service);
    if (s == nullptr || !s->supportsChat()) return ResultQS::ok();
    const std::string account = platform_->session().accountOf(service);
    const auto file = s->dataDir() / "chat" / PathQS::fromUtf8(account + ".json");
    return chat_->addIdentity(AccountRefQS(service, account), s->nicknameOf(account), file);
}

ResultQS ClientQS::login(const std::string& service, const std::string& account, const std::string& password) {
    const ResultQS r = platform_->login(service, account, password);
    if (!r) return r;
    ServiceBaseQS* s = platform_->service(service);
    if (s != nullptr && s->supportsChat()) {
        const ResultQS online = chat_->addIdentity(AccountRefQS(service, account), s->nicknameOf(account),
                                                   s->dataDir() / "chat" / PathQS::fromUtf8(account + ".json"));
        if (!online) {
            platform_->logout(service);
            return online;  // 同一账号不能在两个窗口同时登录
        }
    }
    return r;
}

ResultQS ClientQS::confirmPending(const std::string& service) {
    const ResultQS r = platform_->confirmPending(service);
    if (r) {
        const auto online = bringChatOnline(service);
        if (!online) { platform_->logout(service); return online; }
    }
    return r;
}

void ClientQS::logout(const std::string& service) {
    const std::string account = platform_->session().accountOf(service);
    if (!account.empty()) chat_->removeIdentity(AccountRefQS(service, account));
    platform_->logout(service);
}

void ClientQS::logoutAll() {
    for (const auto& s : platform_->services()) {
        const std::string account = platform_->session().accountOf(s->id());
        if (!account.empty()) chat_->removeIdentity(AccountRefQS(s->id(), account));
    }
    platform_->logoutAll();
}

std::string ClientQS::accountOf(const std::string& service) const {
    return platform_->session().isLoggedIn(service) ? platform_->session().accountOf(service) : std::string();
}

AccountRefQS ClientQS::me(const std::string& service) const { return AccountRefQS(service, accountOf(service)); }

bool ClientQS::chatReady(const std::string& service) const {
    const std::string account = accountOf(service);
    return !account.empty() && chat_->hasIdentity(AccountRefQS(service, account));
}

ChatHistoryQS* ClientQS::history(const std::string& service) { return chat_->history(me(service)); }

bool ClientQS::isOnline(const std::string& service, const std::string& account) const {
    return chat_->isOnline(AccountRefQS(service, account));
}

std::string ClientQS::displayNameOf(const std::string& service, const std::string& account) const {
    ServiceBaseQS* s = platform_->service(service);
    if (s == nullptr) return account;
    if (const FriendCapabilityQS* f = s->friendCapability()) {
        for (const auto& view : f->friendsOf(accountOf(service))) {
            if (view.account == account) return view.displayName();
        }
    }
    return s->nicknameOf(account);
}

std::vector<ConversationViewQS> ClientQS::conversations(const std::string& service) {
    std::vector<ConversationViewQS> result;
    ChatHistoryQS* h = history(service);
    ServiceBaseQS* s = platform_->service(service);
    if (h == nullptr || s == nullptr) return result;
    for (const ConversationQS* c : h->conversations()) {
        ConversationViewQS v;
        v.key = c->key;
        v.isGroup = c->key.rfind("g:", 0) == 0;
        v.peer = c->key.substr(2);
        v.title = c->title;
        if (v.isGroup) {
            GroupQS g;
            if (s->groupCapability() && s->groupCapability()->findGroup(v.peer, g)) v.title = g.name();
        } else {
            v.title = displayNameOf(service, v.peer);
            v.online = isOnline(service, v.peer);
        }
        v.preview = c->preview();
        v.time = c->lastTime();
        v.unread = c->unread;
        result.push_back(v);
    }
    return result;
}

ChatMessageQS ClientQS::sendToFriend(const std::string& service, const std::string& friendAccount, const std::string& text) {
    auto* s = platform_->service(service);
    if (!s || !s->supportsChat() || !s->friendCapability() || !s->account(accountOf(service)) ||
        !s->friendCapability()->areFriends(accountOf(service), friendAccount)) return {};
    return chat_->sendPrivate(me(service), friendAccount, displayNameOf(service, friendAccount), text);
}

ResultQS ClientQS::sendToGroup(const std::string& service, const std::string& groupId, const std::string& text,
                               ChatMessageQS& out) {
    ServiceBaseQS* s = platform_->service(service);
    GroupQS g;
    if (s == nullptr || s->groupCapability() == nullptr || !s->groupCapability()->findGroup(groupId, g)) {
        return ResultQS::fail("群不存在");
    }
    if (!g.hasMember(accountOf(service))) return ResultQS::fail("你已不在该群中");
    std::vector<std::string> members;
    for (const auto& m : g.members()) members.push_back(m.account());
    out = chat_->sendGroup(me(service), groupId, g.name(), members, text);
    if (out.id.empty()) return ResultQS::fail("发送失败：聊天服务未就绪或聊天记录无法保存");
    return ResultQS::ok();
}

std::vector<ChatEventQS> ClientQS::pump(bool& dataChanged) {
    dataChanged = false;
    std::vector<ChatEventQS> events = chat_->poll();
    const std::int64_t now = DateQS::nowMillis();
    if (now - lastTick_ >= 1000) {
        lastTick_ = now;
        auto more = chat_->tick();
        events.insert(events.end(), more.begin(), more.end());
    }
    if (now - lastRefresh_ >= 800) {
        lastRefresh_ = now;
        std::vector<AccountRefQS> previous;
        for (const auto& s : platform_->services()) {
            if (platform_->session().isLoggedIn(s->id())) previous.emplace_back(s->id(), accountOf(s->id()));
        }
        dataChanged = platform_->refreshAll();
        for (const auto& ref : previous) {
            if (!platform_->session().isLoggedIn(ref.service())) chat_->removeIdentity(ref);
        }
    }
    return events;
}

}  // namespace qs

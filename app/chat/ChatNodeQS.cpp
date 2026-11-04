#include "ChatNodeQS.h"

#include "DateQS.h"
#include "PathQS.h"

#include <algorithm>
#include <random>

#ifdef _WIN32
#include <process.h>
#define QS_GETPID _getpid
#else
#include <unistd.h>
#define QS_GETPID getpid
#endif

namespace qs {

ChatNodeQS::ChatNodeQS(const std::string& dataRootUtf8)
    : presence_(PathQS::fromUtf8(dataRootUtf8) / "_presence"),
      identityLockRoot_(PathQS::fromUtf8(dataRootUtf8) / "_presence"), pid_(static_cast<long long>(QS_GETPID())) {}

ChatNodeQS::~ChatNodeQS() { stop(); }

ResultQS ChatNodeQS::start(bool allowLan) {
    if (running_) return ResultQS::ok();
    if (!runtime_.ok()) return ResultQS::fail("网络库初始化失败（WSAStartup）");
    std::string error;
    listener_ = SocketQS::listenOn(allowLan ? "0.0.0.0" : "127.0.0.1", 0, error);
    if (!listener_.valid()) return ResultQS::fail("聊天服务启动失败：" + error);
    port_ = listener_.localPort();
    running_ = true;
    acceptThread_ = std::thread(&ChatNodeQS::acceptLoop, this);
    return ResultQS::ok("聊天节点已在端口 " + std::to_string(port_) + " 上线");
}

void ChatNodeQS::stop() {
    if (!running_) return;
    running_ = false;
    if (acceptThread_.joinable()) acceptThread_.join();
    listener_.close();
    for (const auto& identity : identities_) {
        presence_.withdraw(identity.first, pid_, port_);
        identity.second.history->save();
    }
    identities_.clear(); // releases lifetime identity claims only after history is closed
    std::map<int, std::unique_ptr<PeerLinkQS>> links;
    {
        std::lock_guard<std::mutex> lock(linksMutex_);
        links.swap(links_);
        endpointLinks_.clear();
    }
    links.clear();  // 析构时逐个停止读线程
}

void ChatNodeQS::acceptLoop() {
    while (running_) {
        SocketQS client = listener_.acceptFor(300);
        if (client.valid()) addLink(std::move(client));
    }
}

int ChatNodeQS::addLink(SocketQS socket) {
    std::lock_guard<std::mutex> lock(linksMutex_);
    const int id = nextLinkId_++;
    auto link = std::make_unique<PeerLinkQS>(id, std::move(socket), [this](int linkId, std::string payload) {
        std::lock_guard<std::mutex> inboxLock(inboxMutex_);
        inbox_.emplace_back(linkId, std::move(payload));
    });
    link->start();
    links_[id] = std::move(link);
    return id;
}

std::string ChatNodeQS::nodeEndpoint() const { return "127.0.0.1:" + std::to_string(port_) + "#" + std::to_string(pid_); }

bool ChatNodeQS::sendOnLink(int linkId, const std::string& payload) {
    std::lock_guard<std::mutex> lock(linksMutex_);
    const auto it = links_.find(linkId);
    return it != links_.end() && it->second->send(payload);
}

void ChatNodeQS::sendHello(int linkId, bool reply) {
    JsonQS identities = JsonQS::array();
    for (const auto& identity : identities_) {
        identities.push_back(JsonQS{{"service", identity.first.service()},
                                    {"account", identity.first.account()},
                                    {"nickname", identity.second.nickname}});
    }
    sendOnLink(linkId, JsonQS{{"type", "hello"}, {"node", nodeEndpoint()}, {"identities", identities}, {"reply", reply}}.dump());
}

void ChatNodeQS::broadcastRoster() {
    std::vector<int> ids;
    {
        std::lock_guard<std::mutex> lock(linksMutex_);
        for (const auto& link : links_) ids.push_back(link.first);
    }
    for (int id : ids) sendHello(id, true);
}

int ChatNodeQS::linkFor(const AccountRefQS& target) {
    PresenceQS p;
    std::string endpoint;
    if (presence_.lookup(target, p)) {
        if (p.pid == pid_ && p.port == port_) return 0;  // 同一节点内的账号，不需要网络
        endpoint = p.host + ":" + std::to_string(p.port) + "#" + std::to_string(p.pid);
    } else {
        const auto it = remoteRoster_.find(target);
        if (it == remoteRoster_.end()) return -1;  // 对方不在线
        endpoint = it->second;
    }
    {
        std::lock_guard<std::mutex> lock(linksMutex_);
        const auto it = endpointLinks_.find(endpoint);
        if (it != endpointLinks_.end()) {
            const auto link = links_.find(it->second);
            if (link != links_.end() && link->second->alive()) return it->second;
        }
    }
    if (p.port == 0) return -1;
    std::string error;
    SocketQS socket = SocketQS::connectTo(p.host, p.port, 800, error);
    if (!socket.valid()) return -1;
    const int id = addLink(std::move(socket));
    {
        std::lock_guard<std::mutex> lock(linksMutex_);
        endpointLinks_[endpoint] = id;
        links_[id]->endpoint = endpoint;
    }
    sendHello(id, false);
    return id;
}

ResultQS ChatNodeQS::connectTo(const std::string& host, unsigned short port) {
    std::string error;
    SocketQS socket = SocketQS::connectTo(host, port, 2000, error);
    if (!socket.valid()) return ResultQS::fail(error);
    const int id = addLink(std::move(socket));
    sendHello(id, false);
    return ResultQS::ok("已连接 " + host + ":" + std::to_string(port) + "，正在交换在线账号");
}

ResultQS ChatNodeQS::addIdentity(const AccountRefQS& me, const std::string& nickname,
                                 const std::filesystem::path& historyFile) {
    if (!running_) return ResultQS::fail("聊天节点未启动");
    if (identities_.count(me)) return ResultQS::ok();
    PresenceQS existing;
    if (presence_.lookup(me, existing) && (existing.pid != pid_ || existing.port != port_)) {
        return ResultQS::fail("该账号已在另一个窗口登录，聊天功能只在那个窗口可用");
    }
    IdentityQS identity;
    identity.claim = std::make_unique<FileLockQS>(identityLockRoot_ /
        PathQS::fromUtf8(me.service() + "__" + me.account() + ".lock"), 0);
    if (!identity.claim->locked()) return ResultQS::fail("该账号正由另一个窗口使用或数据目录无法写入");
    identity.nickname = nickname;
    identity.history = std::make_unique<ChatHistoryQS>(me.account(), historyFile);
    identities_.emplace(me, std::move(identity));
    presence_.publish(PresenceQS{me, nickname, "127.0.0.1", port_, pid_, 0});
    broadcastRoster();
    return ResultQS::ok();
}

void ChatNodeQS::removeIdentity(const AccountRefQS& me) {
    const auto it = identities_.find(me);
    if (it == identities_.end()) return;
    presence_.withdraw(me, pid_, port_);
    it->second.history->save();
    identities_.erase(it);
    broadcastRoster();
}

ChatHistoryQS* ChatNodeQS::history(const AccountRefQS& me) {
    const auto it = identities_.find(me);
    return it == identities_.end() ? nullptr : it->second.history.get();
}

bool ChatNodeQS::isOnline(const AccountRefQS& peer) const {
    return presence_.isOnline(peer) || remoteRoster_.count(peer) != 0;
}

std::string ChatNodeQS::nextMessageId(const AccountRefQS& me) {
    static std::mt19937 engine(std::random_device{}());
    return me.key() + "-" + std::to_string(DateQS::nowMillis()) + "-" + std::to_string(++messageCounter_) + "-" +
           std::to_string(engine() % 100000);
}

bool ChatNodeQS::deliver(ChatMessageQS& m, std::vector<ChatEventQS>* events) {
    bool changed = false;
    const AccountRefQS owner(m.service, m.from);
    for (const std::string& recipient : std::vector<std::string>(m.pendingTo)) {
        const AccountRefQS target(m.service, recipient);
        ChatMessageQS wire = m;
        wire.to = recipient;
        bool sent = false;
        if (const auto local = identities_.find(target); local != identities_.end()) {
            // 同一窗口内托管的另一个账号：直接投递（本地回环），同样写入对方记录。
            ChatMessageQS copy = wire;
            copy.outgoing = false;
            copy.status = MessageStatusQS::Received;
            copy.pendingTo.clear();
            copy.ackedBy.clear();
            const bool accepted = !validator_ || validator_(target, copy);
            const bool fresh = local->second.history->find(copy.id) == nullptr;
            if (accepted && local->second.history->appendDurable(copy, copy.isGroup() ? copy.groupName : copy.fromNick, true)) {
                m.ackedBy.push_back(recipient);
                m.pendingTo.erase(std::remove(m.pendingTo.begin(), m.pendingTo.end(), recipient), m.pendingTo.end());
                sent = true;
                if (fresh && events) events->push_back(ChatEventQS{ChatEventQS::KindQS::MessageReceived, target,
                                                          wire.conversationKey(recipient), wire.id, wire.text});
            }
        } else {
            const int linkId = linkFor(target);
            if (linkId > 0) {
                JsonQS frame = wire.toWire();
                frame["type"] = "msg";
                sent = sendOnLink(linkId, frame.dump());
            }
        }
        if (sent) {
            // TCP send does not prove receipt. Keep the durable outbox until ACK;
            // reconnect/restart/lost-ACK retries use the same id and are deduplicated.
            changed = true;
        }
    }
    if (changed) {
        if (static_cast<int>(m.ackedBy.size()) >= m.recipients) m.status = MessageStatusQS::Delivered;
        else m.status = MessageStatusQS::Sent;
        if (events) events->push_back(ChatEventQS{ChatEventQS::KindQS::StatusChanged, owner, m.conversationKey(m.from), m.id, ""});
    }
    return changed;
}

ChatMessageQS ChatNodeQS::sendPrivate(const AccountRefQS& me, const std::string& peer, const std::string& peerNick,
                                      const std::string& text) {
    ChatMessageQS m;
    const auto identity = identities_.find(me);
    if (identity == identities_.end() || text.empty() || peer == me.account()) return m;
    m.id = nextMessageId(me);
    m.service = me.service();
    m.from = me.account();
    m.fromNick = identity->second.nickname;
    m.to = peer;
    m.text = text;
    m.time = DateQS::nowMillis();
    m.outgoing = true;
    m.pendingTo = {peer};
    m.recipients = 1;
    if (!identity->second.history->appendDurable(m, peerNick, false)) return ChatMessageQS{};
    ChatMessageQS* stored = identity->second.history->find(m.id);
    if (!stored) return ChatMessageQS{};
    deliver(*stored, nullptr);
    identity->second.history->save();
    return *stored;
}

ChatMessageQS ChatNodeQS::sendGroup(const AccountRefQS& me, const std::string& groupId, const std::string& groupName,
                                    const std::vector<std::string>& members, const std::string& text) {
    ChatMessageQS m;
    const auto identity = identities_.find(me);
    if (identity == identities_.end() || text.empty()) return m;
    m.id = nextMessageId(me);
    m.service = me.service();
    m.from = me.account();
    m.fromNick = identity->second.nickname;
    m.groupId = groupId;
    m.groupName = groupName;
    m.text = text;
    m.time = DateQS::nowMillis();
    m.outgoing = true;
    for (const auto& member : members) {
        if (member != me.account() && std::find(m.pendingTo.begin(), m.pendingTo.end(), member) == m.pendingTo.end())
            m.pendingTo.push_back(member);
    }
    m.recipients = static_cast<int>(m.pendingTo.size());
    if (m.recipients == 0) m.status = MessageStatusQS::Delivered;
    if (!identity->second.history->appendDurable(m, groupName, false)) return ChatMessageQS{};
    ChatMessageQS* stored = identity->second.history->find(m.id);
    if (!stored) return ChatMessageQS{};
    deliver(*stored, nullptr);
    identity->second.history->save();
    return *stored;
}

void ChatNodeQS::handleFrame(int linkId, const std::string& payload, std::vector<ChatEventQS>& events) {
    const JsonQS j = JsonQS::parse(payload, nullptr, false);
    if (j.is_discarded() || !j.is_object()) return;
    const std::string type = j.value("type", std::string());
    if (type == "hello") {
        const std::string endpoint = j.value("node", std::string());
        if (endpoint.empty() || !j.contains("identities") || !j["identities"].is_array()) return;
        std::set<AccountRefQS> declared;
        for (const auto& identity : j["identities"]) {
            const AccountRefQS ref(identity.value("service", std::string()), identity.value("account", std::string()));
            if (ref.service().empty() || ref.account().empty()) return;
            declared.insert(ref);
        }
        {
            std::lock_guard<std::mutex> lock(linksMutex_);
            if (links_.count(linkId)) links_[linkId]->endpoint = endpoint;
            endpointLinks_[endpoint] = linkId;
        }
        for (auto it = remoteRoster_.begin(); it != remoteRoster_.end();) {
            it = it->second == endpoint ? remoteRoster_.erase(it) : std::next(it);
        }
        linkIdentities_[linkId] = declared;
        for (const auto& ref : declared) remoteRoster_[ref] = endpoint;
        if (!j.value("reply", false)) sendHello(linkId, true);
        events.push_back(ChatEventQS{ChatEventQS::KindQS::PresenceChanged, {}, "", "", ""});
        return;
    }
    if (type == "msg") {
        const ChatMessageQS m = ChatMessageQS::fromWire(j);
        if (m.id.empty() || m.text.empty() || !linkIdentities_[linkId].count(AccountRefQS(m.service, m.from))) return;
        const AccountRefQS owner(m.service, m.to);
        const auto identity = identities_.find(owner);
        if (identity == identities_.end()) return;  // 该账号已不在本窗口
        if (validator_ && !validator_(owner, m)) return;
        ChatMessageQS stored = m;
        stored.outgoing = false;
        stored.status = MessageStatusQS::Received;
        const bool fresh = identity->second.history->find(m.id) == nullptr;
        if (!identity->second.history->appendDurable(stored, m.isGroup() ? m.groupName : m.fromNick, true)) return;
        // ACK only a durable message; matching duplicates still get an ACK.
        sendOnLink(linkId, JsonQS{{"type", "ack"}, {"id", m.id}, {"service", m.service}, {"from", m.to}, {"to", m.from}}.dump());
        if (fresh) {
            events.push_back(ChatEventQS{ChatEventQS::KindQS::MessageReceived, owner, stored.conversationKey(owner.account()),
                                         m.id, m.text});
        }
        return;
    }
    if (type == "ack") {
        const AccountRefQS owner(j.value("service", std::string()), j.value("to", std::string()));
        ChatHistoryQS* h = history(owner);
        if (h == nullptr) return;
        ChatMessageQS* m = h->find(j.value("id", std::string()));
        if (m == nullptr || !m->outgoing) return;
        const std::string acker = j.value("from", std::string());
        if (!linkIdentities_[linkId].count(AccountRefQS(owner.service(), acker)) ||
            std::find(m->pendingTo.begin(), m->pendingTo.end(), acker) == m->pendingTo.end()) return;
        const ChatMessageQS before = *m;
        if (std::find(m->ackedBy.begin(), m->ackedBy.end(), acker) == m->ackedBy.end()) m->ackedBy.push_back(acker);
        m->pendingTo.erase(std::remove(m->pendingTo.begin(), m->pendingTo.end(), acker), m->pendingTo.end());
        m->status = static_cast<int>(m->ackedBy.size()) >= m->recipients ? MessageStatusQS::Delivered : MessageStatusQS::Sent;
        if (!h->save()) { *m = before; return; }
        events.push_back(ChatEventQS{ChatEventQS::KindQS::StatusChanged, owner, m->conversationKey(owner.account()), m->id, ""});
    }
}

void ChatNodeQS::purgeDeadLinks(std::vector<ChatEventQS>& events) {
    std::vector<std::unique_ptr<PeerLinkQS>> dead;
    {
        std::lock_guard<std::mutex> lock(linksMutex_);
        for (auto it = links_.begin(); it != links_.end();) {
            if (it->second->alive()) {
                ++it;
                continue;
            }
            const std::string endpoint = it->second->endpoint;
            for (auto e = endpointLinks_.begin(); e != endpointLinks_.end();) {
                e = e->second == it->first ? endpointLinks_.erase(e) : std::next(e);
            }
            for (auto r = remoteRoster_.begin(); r != remoteRoster_.end();) {
                r = (!endpoint.empty() && r->second == endpoint) ? remoteRoster_.erase(r) : std::next(r);
            }
            dead.push_back(std::move(it->second));
            linkIdentities_.erase(it->first);
            it = links_.erase(it);
        }
    }
    if (!dead.empty()) events.push_back(ChatEventQS{ChatEventQS::KindQS::PresenceChanged, {}, "", "", ""});
    dead.clear();  // 在锁外停止线程，避免死锁
}

std::vector<ChatEventQS> ChatNodeQS::poll() {
    std::vector<ChatEventQS> events;
    std::deque<std::pair<int, std::string>> inbox;
    {
        std::lock_guard<std::mutex> lock(inboxMutex_);
        inbox.swap(inbox_);
    }
    for (const auto& frame : inbox) {
        try { handleFrame(frame.first, frame.second, events); }
        catch (const std::exception&) { /* malformed typed JSON must not escape into the UI event loop */ }
    }
    purgeDeadLinks(events);
    return events;
}

std::vector<ChatEventQS> ChatNodeQS::tick() {
    std::vector<ChatEventQS> events;
    const std::int64_t now = DateQS::nowMillis();
    if (now - lastHeartbeat_ >= 2000) {
        lastHeartbeat_ = now;
        for (const auto& identity : identities_) {
            presence_.publish(PresenceQS{identity.first, identity.second.nickname, "127.0.0.1", port_, pid_, 0});
        }
    }
    // 离线消息补发：对方上线后自动送达
    for (auto& identity : identities_) {
        bool changed = false;
        for (ChatMessageQS* m : identity.second.history->outbox()) {
            changed = deliver(*m, &events) || changed;
        }
        if (changed) identity.second.history->save();
    }
    return events;
}

}  // namespace qs

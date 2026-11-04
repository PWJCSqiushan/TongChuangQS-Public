#include "PlatformQS.h"

#include "DateQS.h"
#include "PathQS.h"
#include "ServiceRegistryQS.h"

#include <algorithm>
#include <set>

namespace qs {

PlatformQS::PlatformQS(std::string dataRootUtf8) : dataRoot_(std::move(dataRootUtf8)) {}

PlatformQS::~PlatformQS() = default;

ResultQS PlatformQS::start() {
    auto staged = ServiceRegistryQS::instance().createAll();
    // A reduced build cannot validate bindings owned by an absent service.
    // Inspect storage markers only; never parse that service's private data.
    const auto root = PathQS::fromUtf8(dataRoot_);
    std::error_code error;
    try {
        if (std::filesystem::exists(root, error)) {
            for (const auto& entry : std::filesystem::directory_iterator(root)) {
                if (!entry.is_directory()) continue;
                if (!std::filesystem::exists(entry.path() / "revision.json") &&
                    !std::filesystem::exists(entry.path() / "accounts.json")) continue;
                const auto id = PathQS::toUtf8(entry.path().filename());
                const bool compiled = std::any_of(staged.begin(), staged.end(),
                    [&](const auto& service) { return service->id() == id; });
                if (!compiled) return ResultQS::fail("数据目录包含当前构建未启用的服务（" + id +
                    "）。为保护账号绑定，请使用完整版本或新的独立数据目录");
            }
        } else if (error) { return ResultQS::fail("无法读取数据目录：" + error.message()); }
    } catch (const std::filesystem::filesystem_error& e) {
        return ResultQS::fail(std::string("无法读取数据目录：") + e.what());
    }
    for (auto& s : staged) {
        s->attach(*this, dataRoot_);
    }
    for (auto& s : staged) {
        const auto loaded = s->load();
        if (!loaded) return ResultQS::fail(s->name() + "：" + loaded.message());
    }
    // Legacy stores did not record retired IDs. Preserve every dangling binding
    // before allowing new registrations, so an old link cannot acquire a new owner.
    for (auto& target : staged) {
        std::vector<std::string> referenced;
        for (const auto& source : staged) for (const auto* account : source->accounts().all())
            for (const auto& ref : account->links()) if (ref.service() == target->id()) referenced.push_back(ref.account());
        const auto reserved = target->reserveMissingLinkedIds(referenced);
        if (!reserved) return reserved;
    }
    services_ = std::move(staged);
    session_.clear();
    return ResultQS::ok();
}

ServiceBaseQS* PlatformQS::service(const std::string& id) const {
    for (const auto& s : services_) {
        if (s->id() == id) return s.get();
    }
    return nullptr;
}

std::vector<std::string> PlatformQS::serviceIds() const {
    std::vector<std::string> ids;
    for (const auto& s : services_) ids.push_back(s->id());
    return ids;
}

ServiceReplyQS PlatformQS::send(const ServiceRequestQS& request) {
    ServiceReplyQS reply;
    ServiceBaseQS* target = service(request.to);
    if (target == nullptr) {
        reply = ServiceReplyQS::failure("平台上没有服务：" + request.to);
    } else {
        try {
            reply = target->handleRequest(request);
        } catch (const std::exception& e) {
            reply = ServiceReplyQS::failure(std::string("服务处理请求时出错：") + e.what());
        }
    }
    busLog_.push_back(BusLogEntryQS{DateQS::nowMillis(), request.from, request.to, request.kind, reply.ok, reply.message});
    while (busLog_.size() > 200) busLog_.pop_front();
    return reply;
}

bool PlatformQS::refreshAll() {
    bool changed = false;
    for (auto& s : services_) {
        changed = s->reloadIfChanged() || changed;
        const auto account = session_.accountOf(s->id());
        if (!account.empty() && s->storageError().empty() && s->account(account) == nullptr) {
            session_.logout(s->id()); changed = true;
        }
    }
    return changed;
}

ResultQS PlatformQS::login(const std::string& serviceId, const std::string& account, const std::string& password) {
    ServiceBaseQS* s = service(serviceId);
    if (s == nullptr) return ResultQS::fail("服务不存在");
    const ResultQS r = s->login(account, password);
    if (!r) return r;
    session_.markLoggedIn(serviceId, account);
    discoverLinkedServices(AccountRefQS(serviceId, account));
    return r;
}

ResultQS PlatformQS::confirmPending(const std::string& serviceId) {
    if (!session_.isPending(serviceId)) return ResultQS::fail("该服务没有待确认的登录");
    const std::string account = session_.accountOf(serviceId);
    ServiceBaseQS* s = service(serviceId);
    if (s == nullptr || s->account(account) == nullptr) {
        session_.logout(serviceId);
        return ResultQS::fail("该账号已不存在");
    }
    session_.markLoggedIn(serviceId, account);
    discoverLinkedServices(AccountRefQS(serviceId, account));
    return ResultQS::ok("已确认，以 " + account + " 进入" + s->name());
}

void PlatformQS::logout(const std::string& serviceId) { session_.logout(serviceId); }
void PlatformQS::logoutAll() { session_.clear(); }

void PlatformQS::discoverLinkedServices(const AccountRefQS& loggedIn) {
    for (const AccountRefQS& ref : linkedAccounts(loggedIn)) {
        if (ref.service() != loggedIn.service() && !session_.isLoggedIn(ref.service())) {
            session_.markPending(ref.service(), ref.account());
        }
    }
}

std::vector<AccountRefQS> PlatformQS::directLinks(const AccountRefQS& ref) {
    std::vector<AccountRefQS> result;
    for (const auto& s : services_) {
        // 每个服务只回答"它自己知道的"关联关系，平台把答案汇总。
        const ServiceReplyQS reply = send(ServiceRequestQS{"platform", s->id(), "linked_accounts", JsonQS{{"ref", ref.toJson()}}});
        if (!reply.ok) continue;
        for (const auto& item : reply.payload.value("refs", JsonQS::array())) {
            result.push_back(AccountRefQS::fromJson(item));
        }
    }
    return result;
}

std::vector<AccountRefQS> PlatformQS::linkedAccounts(const AccountRefQS& ref) {
    std::set<AccountRefQS> seen{ref};
    std::vector<AccountRefQS> queue{ref};
    std::vector<AccountRefQS> result;
    for (std::size_t i = 0; i < queue.size() && i < 32; ++i) {
        for (const AccountRefQS& next : directLinks(queue[i])) {
            ServiceBaseQS* s = service(next.service());
            if (s == nullptr || s->account(next.account()) == nullptr || seen.count(next)) continue;
            seen.insert(next);
            queue.push_back(next);
            result.push_back(next);
        }
    }
    return result;
}

std::string PlatformQS::linkedAccountIn(const AccountRefQS& me, const std::string& targetService) {
    if (me.service() == targetService) return me.account();
    for (const AccountRefQS& ref : linkedAccounts(me)) {
        if (ref.service() == targetService) return ref.account();
    }
    return {};
}

std::vector<std::string> PlatformQS::linkedRegistrationTargets(const std::string& sourceService) const {
    std::vector<std::string> targets;
    for (const auto& s : services_) {
        if (s->id() != sourceService && s->acceptsLinkedRegistrationFrom(sourceService)) targets.push_back(s->id());
    }
    return targets;
}

std::vector<std::pair<std::string, ResultQS>> PlatformQS::registerLinked(const AccountRefQS& source,
                                                                         const std::string& password,
                                                                         const std::vector<std::string>& targets) {
    std::vector<std::pair<std::string, ResultQS>> results;
    ServiceBaseQS* origin = service(source.service());
    const ServiceAccountQS* account = origin == nullptr ? nullptr : origin->account(source.account());
    if (account == nullptr) return results;
    for (const std::string& target : targets) {
        const ServiceReplyQS reply = send(ServiceRequestQS{
            source.service(), target, "register_linked",
            JsonQS{{"source", source.toJson()}, {"profile", account->profile().toJson()}, {"password", password}}});
        results.emplace_back(target, reply.ok ? ResultQS::ok(reply.message) : ResultQS::fail(reply.message));
    }
    return results;
}

std::vector<FriendCandidateQS> PlatformQS::recommendFriends(const std::string& targetService, const std::string& me,
                                                            const std::string& sourceService, std::string& hint) {
    std::vector<FriendCandidateQS> result;
    ServiceBaseQS* target = service(targetService);
    ServiceBaseQS* source = service(sourceService);
    if (target == nullptr || source == nullptr || target->friendCapability() == nullptr) {
        hint = "该服务不支持好友推荐";
        return result;
    }
    const std::string mine = linkedAccountIn(AccountRefQS(targetService, me), sourceService);
    if (mine.empty()) {
        hint = "你还没有关联" + source->name() + "账号，无法按" + source->name() + "好友推荐";
        return result;
    }
    const ServiceReplyQS reply =
        send(ServiceRequestQS{targetService, sourceService, "friends_of", JsonQS{{"account", mine}}});
    if (!reply.ok) {
        hint = reply.message;
        return result;
    }
    const FriendCapabilityQS* friends = target->friendCapability();
    for (const auto& f : reply.payload.value("friends", JsonQS::array())) {
        const std::string sourceFriend = f.value("account", std::string());
        const std::string candidate = linkedAccountIn(AccountRefQS(sourceService, sourceFriend), targetService);
        if (candidate.empty() || candidate == me || friends->areFriends(me, candidate)) continue;
        result.push_back(FriendCandidateQS{candidate, target->nicknameOf(candidate), sourceFriend,
                                           "你的" + source->name() + "好友 " + f.value("nickname", std::string()) + "（" +
                                               sourceFriend + "）"});
    }
    hint = result.empty() ? "没有可推荐的新好友（对方可能已是好友，或尚未开通" + target->name() + "）"
                          : "找到 " + std::to_string(result.size()) + " 位可添加的好友";
    return result;
}

std::vector<CommonFriendQS> PlatformQS::commonFriends(const AccountRefQS& me, const std::string& otherService,
                                                      std::string& hint) {
    std::vector<CommonFriendQS> result;
    ServiceBaseQS* first = service(me.service());
    ServiceBaseQS* second = service(otherService);
    if (first == nullptr || second == nullptr || first->friendCapability() == nullptr ||
        second->friendCapability() == nullptr) {
        hint = "两个服务都需要有好友功能";
        return result;
    }
    const std::string other = linkedAccountIn(me, otherService);
    if (other.empty()) {
        hint = "你还没有关联" + second->name() + "账号";
        return result;
    }
    const auto secondFriends = second->friendCapability()->friendsOf(other);
    for (const auto& f : first->friendCapability()->friendsOf(me.account())) {
        const std::string mapped = linkedAccountIn(AccountRefQS(me.service(), f.account), otherService);
        if (mapped.empty()) continue;
        const bool both = std::any_of(secondFriends.begin(), secondFriends.end(),
                                      [&](const FriendViewQS& s) { return s.account == mapped; });
        if (both) {
            result.push_back(CommonFriendQS{f.nickname, AccountRefQS(me.service(), f.account),
                                            AccountRefQS(otherService, mapped)});
        }
    }
    hint = "共同好友 " + std::to_string(result.size()) + " 人";
    return result;
}

}  // namespace qs

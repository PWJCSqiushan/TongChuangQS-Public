#include "StandardGroupsQS.h"

#include "DateQS.h"
#include "GroupPolicyRegistryQS.h"
#include "ServiceBaseQS.h"

namespace qs {

StandardGroupsQS::StandardGroupsQS(ServiceBaseQS& owner, std::string defaultPolicy, EligibilityQS eligibility)
    : owner_(owner), defaultPolicy_(std::move(defaultPolicy)), eligibility_(std::move(eligibility)) {}

std::vector<GroupQS> StandardGroupsQS::groupsOf(const std::string& me) const {
    std::vector<GroupQS> result;
    for (const auto& g : groups_) {
        if (g.second.hasMember(me)) result.push_back(g.second);
    }
    return result;
}

std::vector<GroupQS> StandardGroupsQS::directory() const {
    std::vector<GroupQS> result;
    for (const auto& g : groups_) result.push_back(g.second);
    return result;
}

bool StandardGroupsQS::findGroup(const std::string& groupId, GroupQS& out) const {
    const auto it = groups_.find(groupId);
    if (it == groups_.end()) return false;
    out = it->second;
    return true;
}

int StandardGroupsQS::pendingCount(const std::string& me) const {
    int count = 0;
    for (const auto& entry : groups_) {
        const GroupQS& g = entry.second;
        GroupRoleQS role = GroupRoleQS::Member;
        if (g.effectiveRole(me, role) && g.policy().canApprove(role) &&
            g.policy().joinMode() == JoinModeQS::Application) {
            count += static_cast<int>(g.applications().size());
        }
        if (g.hasInvitation(me)) ++count;
    }
    return count;
}

ResultQS StandardGroupsQS::withGroup(const std::string& groupId, const std::string& actor, const std::function<ResultQS(GroupQS&)>& action) {
    return owner_.transact([&]() -> ResultQS {
        if (owner_.account(actor) == nullptr) return ResultQS::fail("当前账号不存在，请重新登录");
        const auto it = groups_.find(groupId);
        if (it == groups_.end()) return ResultQS::fail("群 " + groupId + " 不存在");
        return action(it->second);
    });
}

ResultQS StandardGroupsQS::createGroup(const std::string& me, const std::string& name, std::string& newId) {
    return owner_.transact([&]() -> ResultQS {
        if (owner_.account(me) == nullptr) return ResultQS::fail("当前账号不存在，请重新登录");
        if (name.empty()) return ResultQS::fail("请填写群名称");
        long long next = 1001;
        while (groups_.count(std::to_string(next))) ++next;
        newId = std::to_string(next);
        groups_.emplace(newId, GroupQS(newId, name, me, GroupPolicyRegistryQS::instance().create(defaultPolicy_)));
        return ResultQS::ok("群「" + name + "」创建成功，群号 " + newId);
    });
}

ResultQS StandardGroupsQS::requestJoin(const std::string& me, const std::string& groupId, const std::string& note) {
    return withGroup(groupId, me, [&](GroupQS& g) {
        const bool eligible = eligibility_ ? eligibility_(g, me) : true;
        return g.requestJoin(me, note, eligible);
    });
}

ResultQS StandardGroupsQS::reviewApplication(const std::string& me, const std::string& groupId,
                                             const std::string& applicant, bool accept) {
    return withGroup(groupId, me, [&](GroupQS& g) { return g.reviewApplication(me, applicant, accept); });
}

ResultQS StandardGroupsQS::invite(const std::string& me, const std::string& groupId, const std::string& invitee) {
    if (owner_.account(invitee) == nullptr) return ResultQS::fail("没有找到" + owner_.accountLabel() + " " + invitee);
    return withGroup(groupId, me, [&](GroupQS& g) { if (!owner_.account(invitee)) return ResultQS::fail("被邀请账号已不存在"); return g.invite(me, invitee); });
}

ResultQS StandardGroupsQS::answerInvitation(const std::string& me, const std::string& groupId, bool accept) {
    return withGroup(groupId, me, [&](GroupQS& g) { return g.answerInvitation(me, accept); });
}

ResultQS StandardGroupsQS::leave(const std::string& me, const std::string& groupId) {
    return owner_.transact([&]() -> ResultQS {
        const auto it = groups_.find(groupId);
        if (it == groups_.end()) return ResultQS::fail("群不存在");
        const bool lastMember = it->second.members().size() == 1 && it->second.hasMember(me);
        const ResultQS r = it->second.leave(me);
        if (r && lastMember) {
            groups_.erase(it);
            return ResultQS::ok("你是最后一名成员，群已解散");
        }
        return r;
    });
}

ResultQS StandardGroupsQS::kick(const std::string& me, const std::string& groupId, const std::string& target) {
    return withGroup(groupId, me, [&](GroupQS& g) { return g.kick(me, target); });
}

ResultQS StandardGroupsQS::setAdmin(const std::string& me, const std::string& groupId, const std::string& target,
                                    bool admin) {
    return withGroup(groupId, me, [&](GroupQS& g) { return g.setAdmin(me, target, admin); });
}

ResultQS StandardGroupsQS::transferOwner(const std::string& me, const std::string& groupId, const std::string& target) {
    return withGroup(groupId, me, [&](GroupQS& g) { return g.transferOwner(me, target); });
}

ResultQS StandardGroupsQS::createSubgroup(const std::string& me, const std::string& groupId, const std::string& name,
                                          const std::vector<std::string>& members) {
    return withGroup(groupId, me, [&](GroupQS& g) { return g.createSubgroup(me, name, members); });
}

ResultQS StandardGroupsQS::setAnnouncement(const std::string& me, const std::string& groupId, const std::string& text) {
    return withGroup(groupId, me, [&](GroupQS& g) { return g.setAnnouncement(me, text); });
}

ResultQS StandardGroupsQS::switchPolicy(const std::string& me, const std::string& groupId, const std::string& policyKind) {
    return withGroup(groupId, me, [&](GroupQS& g) {
        return g.switchPolicy(me, GroupPolicyRegistryQS::instance().create(policyKind));
    });
}

GroupQS& StandardGroupsQS::seedGroup(const std::string& id, const std::string& name, const std::string& policyKind,
                                     const std::vector<std::string>& members, const std::string& announcement) {
    GroupQS group(id, name, members.front(), GroupPolicyRegistryQS::instance().create(policyKind));
    for (std::size_t i = 1; i < members.size(); ++i) {
        group.addMemberDirect(members[i], GroupRoleQS::Member, "2026-09-01");
    }
    if (!announcement.empty()) {
        group.setAnnouncement(members.front(), announcement);
    }
    groups_[id] = std::move(group);
    return groups_[id];
}

void StandardGroupsQS::forget(const std::string& account) {
    for (auto it = groups_.begin(); it != groups_.end();) {
        it->second.removeEverywhere(account);
        it->second.ensureOwner();
        it = it->second.members().empty() ? groups_.erase(it) : std::next(it);
    }
}

JsonQS StandardGroupsQS::toJson() const {
    JsonQS array = JsonQS::array();
    for (const auto& g : groups_) array.push_back(g.second.toJson());
    return array;
}

void StandardGroupsQS::fromJson(const JsonQS& j) {
    groups_.clear();
    if (!j.is_array()) return;
    for (const auto& item : j) {
        GroupQS g = GroupQS::fromJson(item);
        const std::string id = g.id();
        groups_.emplace(id, std::move(g));
    }
}

}  // namespace qs

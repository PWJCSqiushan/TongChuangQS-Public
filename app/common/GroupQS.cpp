#include "GroupQS.h"

#include "DateQS.h"
#include "GroupPolicyRegistryQS.h"
#include "QQGroupPolicyQS.h"

#include <algorithm>

namespace qs {

GroupQS::GroupQS() : policy_(std::make_unique<QQGroupPolicyQS>()) {}

GroupQS::GroupQS(std::string id, std::string name, const std::string& owner, std::unique_ptr<GroupPolicyQS> policy)
    : id_(std::move(id)), name_(std::move(name)), createdOn_(DateQS::today()), policy_(std::move(policy)) {
    if (!policy_) {
        policy_ = std::make_unique<QQGroupPolicyQS>();
    }
    members_.emplace_back(owner, GroupRoleQS::Owner, createdOn_);
}

GroupQS::GroupQS(const GroupQS& other)
    : id_(other.id_),
      name_(other.name_),
      announcement_(other.announcement_),
      createdOn_(other.createdOn_),
      policy_(other.policy_->clone()),
      members_(other.members_),
      applications_(other.applications_),
      invitations_(other.invitations_),
      subgroups_(other.subgroups_),
      nextSubgroup_(other.nextSubgroup_) {}

GroupQS& GroupQS::operator=(const GroupQS& other) {
    if (this != &other) {
        GroupQS copy(other);
        *this = std::move(copy);
    }
    return *this;
}

std::string GroupQS::owner() const {
    for (const auto& m : members_) {
        if (m.role() == GroupRoleQS::Owner) {
            return m.account();
        }
    }
    return {};
}

bool GroupQS::hasMember(const std::string& account) const {
    return std::any_of(members_.begin(), members_.end(), [&](const GroupMemberQS& m) { return m.account() == account; });
}

GroupMemberQS* GroupQS::findMember(const std::string& account) {
    for (auto& m : members_) {
        if (m.account() == account) {
            return &m;
        }
    }
    return nullptr;
}

bool GroupQS::roleOf(const std::string& account, GroupRoleQS& role) const {
    for (const auto& m : members_) {
        if (m.account() == account) {
            role = m.role();
            return true;
        }
    }
    return false;
}

bool GroupQS::effectiveRole(const std::string& account, GroupRoleQS& role) const {
    GroupRoleQS stored = GroupRoleQS::Member;
    if (!roleOf(account, stored)) {
        return false;
    }
    role = policy_->effectiveRole(stored);
    return true;
}

bool GroupQS::hasApplication(const std::string& account) const {
    return std::any_of(applications_.begin(), applications_.end(),
                       [&](const GroupApplicationQS& a) { return a.account == account; });
}

bool GroupQS::hasInvitation(const std::string& account) const {
    return std::any_of(invitations_.begin(), invitations_.end(),
                       [&](const GroupInvitationQS& i) { return i.invitee == account; });
}

int GroupQS::adminCount() const {
    return static_cast<int>(std::count_if(members_.begin(), members_.end(),
                                          [](const GroupMemberQS& m) { return m.role() == GroupRoleQS::Admin; }));
}

ResultQS GroupQS::requestJoin(const std::string& account, const std::string& note, bool eligible) {
    if (hasMember(account)) {
        return ResultQS::fail("你已经是该群成员");
    }
    switch (policy_->joinMode()) {
        case JoinModeQS::InviteOnly:
            return ResultQS::fail(policy_->joinHint());
        case JoinModeQS::Open:
            if (!eligible) {
                return ResultQS::fail(policy_->joinHint());
            }
            addMemberDirect(account, GroupRoleQS::Member, DateQS::today());
            return ResultQS::ok("已加入群「" + name_ + "」");
        case JoinModeQS::Application:
            if (hasApplication(account)) {
                return ResultQS::fail("已提交过申请，请等待审批");
            }
            applications_.push_back(GroupApplicationQS{account, note, DateQS::today()});
            return ResultQS::ok(policy_->joinHint());
    }
    return ResultQS::fail("未知的加入方式");
}

ResultQS GroupQS::reviewApplication(const std::string& actor, const std::string& applicant, bool accept) {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!effectiveRole(actor, role) || !policy_->canApprove(role)) {
        return ResultQS::fail("当前群管理模式下你没有审批权限");
    }
    const auto it = std::find_if(applications_.begin(), applications_.end(),
                                 [&](const GroupApplicationQS& a) { return a.account == applicant; });
    if (it == applications_.end()) {
        return ResultQS::fail("没有找到这条申请");
    }
    applications_.erase(it);
    if (!accept) {
        return ResultQS::ok("已拒绝该申请");
    }
    if (!hasMember(applicant)) {
        addMemberDirect(applicant, GroupRoleQS::Member, DateQS::today());
    }
    return ResultQS::ok("已同意，新成员已入群");
}

ResultQS GroupQS::invite(const std::string& actor, const std::string& invitee) {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!effectiveRole(actor, role)) {
        return ResultQS::fail("你不是该群成员");
    }
    if (!policy_->canInvite(role)) {
        return ResultQS::fail("当前群管理模式下你没有邀请权限");
    }
    if (hasMember(invitee)) {
        return ResultQS::fail("对方已经在群里");
    }
    if (hasInvitation(invitee)) {
        return ResultQS::fail("已经邀请过对方，等待对方确认");
    }
    invitations_.push_back(GroupInvitationQS{invitee, actor, DateQS::today()});
    return ResultQS::ok("邀请已发出，对方确认后即可入群");
}

ResultQS GroupQS::answerInvitation(const std::string& invitee, bool accept) {
    const auto it = std::find_if(invitations_.begin(), invitations_.end(),
                                 [&](const GroupInvitationQS& i) { return i.invitee == invitee; });
    if (it == invitations_.end()) {
        return ResultQS::fail("没有找到这条邀请");
    }
    invitations_.erase(it);
    if (!accept) {
        return ResultQS::ok("已拒绝邀请");
    }
    if (!hasMember(invitee)) {
        addMemberDirect(invitee, GroupRoleQS::Member, DateQS::today());
    }
    applications_.erase(std::remove_if(applications_.begin(), applications_.end(),
                                       [&](const GroupApplicationQS& a) { return a.account == invitee; }),
                        applications_.end());
    return ResultQS::ok("已加入群「" + name_ + "」");
}

ResultQS GroupQS::leave(const std::string& account) {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!roleOf(account, role)) {
        return ResultQS::fail("你不是该群成员");
    }
    if (role == GroupRoleQS::Owner && members_.size() > 1) {
        return ResultQS::fail("群主需要先转让群主身份才能退群");
    }
    removeEverywhere(account);
    return ResultQS::ok("已退出群「" + name_ + "」");
}

ResultQS GroupQS::kick(const std::string& actor, const std::string& target) {
    GroupRoleQS actorRole = GroupRoleQS::Member;
    GroupRoleQS targetRole = GroupRoleQS::Member;
    if (!effectiveRole(actor, actorRole)) {
        return ResultQS::fail("你不是该群成员");
    }
    if (!effectiveRole(target, targetRole)) {
        return ResultQS::fail("对方不在群里");
    }
    if (actor == target) {
        return ResultQS::fail("不能移出自己，请使用退群");
    }
    if (!policy_->canKick(actorRole, targetRole)) {
        return ResultQS::fail("当前群管理模式下你不能移出该成员");
    }
    removeEverywhere(target);
    return ResultQS::ok("已将该成员移出群聊");
}

ResultQS GroupQS::setAdmin(const std::string& actor, const std::string& target, bool admin) {
    GroupRoleQS actorRole = GroupRoleQS::Member;
    if (!effectiveRole(actor, actorRole) || !policy_->canSetAdmin(actorRole)) {
        return ResultQS::fail(policy_->supportsAdmins() ? "只有群主可以设置管理员"
                                                        : "当前群管理模式没有管理员制度");
    }
    GroupMemberQS* member = findMember(target);
    if (member == nullptr) {
        return ResultQS::fail("对方不在群里");
    }
    if (member->role() == GroupRoleQS::Owner) {
        return ResultQS::fail("不能修改群主的角色");
    }
    if (admin) {
        if (member->role() == GroupRoleQS::Admin) {
            return ResultQS::fail("对方已经是管理员");
        }
        if (adminCount() >= policy_->maxAdmins()) {
            return ResultQS::fail("管理员人数已达上限（" + std::to_string(policy_->maxAdmins()) + "人）");
        }
        member->setRole(GroupRoleQS::Admin);
        return ResultQS::ok("已设为管理员");
    }
    if (member->role() != GroupRoleQS::Admin) {
        return ResultQS::fail("对方不是管理员");
    }
    member->setRole(GroupRoleQS::Member);
    return ResultQS::ok("已取消管理员");
}

ResultQS GroupQS::transferOwner(const std::string& actor, const std::string& target) {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!roleOf(actor, role) || role != GroupRoleQS::Owner) {
        return ResultQS::fail("只有群主可以转让群");
    }
    GroupMemberQS* next = findMember(target);
    if (next == nullptr || target == actor) {
        return ResultQS::fail("请选择另一名群成员");
    }
    findMember(actor)->setRole(GroupRoleQS::Member);
    next->setRole(GroupRoleQS::Owner);
    return ResultQS::ok("群主已转让");
}

ResultQS GroupQS::createSubgroup(const std::string& actor, const std::string& name, std::vector<std::string> members) {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!effectiveRole(actor, role)) {
        return ResultQS::fail("你不是该群成员");
    }
    if (!policy_->supportsSubgroups() || !policy_->canCreateSubgroup(role)) {
        return ResultQS::fail("当前群管理模式不支持临时讨论组");
    }
    if (name.empty()) {
        return ResultQS::fail("请填写讨论组名称");
    }
    for (const auto& s : subgroups_) {
        if (s.name == name) {
            return ResultQS::fail("已有同名讨论组");
        }
    }
    if (std::find(members.begin(), members.end(), actor) == members.end()) {
        members.insert(members.begin(), actor);
    }
    for (const auto& m : members) {
        if (!hasMember(m)) {
            return ResultQS::fail("讨论组成员必须是本群成员");
        }
    }
    if (members.size() < 2) {
        return ResultQS::fail("讨论组至少需要2人");
    }
    SubgroupQS sub;
    sub.id = id_ + "-" + std::to_string(nextSubgroup_++);
    sub.name = name;
    sub.creator = actor;
    sub.createdOn = DateQS::today();
    sub.members = std::move(members);
    subgroups_.push_back(std::move(sub));
    return ResultQS::ok("讨论组「" + name + "」已创建");
}

ResultQS GroupQS::setAnnouncement(const std::string& actor, const std::string& text) {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!effectiveRole(actor, role) || !policy_->canEditAnnouncement(role)) {
        return ResultQS::fail("当前群管理模式下你不能修改群公告");
    }
    announcement_ = text;
    return ResultQS::ok("群公告已更新");
}

ResultQS GroupQS::rename(const std::string& actor, const std::string& name) {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!effectiveRole(actor, role) || role != GroupRoleQS::Owner) {
        return ResultQS::fail("只有群主可以修改群名称");
    }
    if (name.empty()) {
        return ResultQS::fail("群名称不能为空");
    }
    name_ = name;
    return ResultQS::ok("群名称已修改");
}

ResultQS GroupQS::switchPolicy(const std::string& actor, std::unique_ptr<GroupPolicyQS> next) {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!roleOf(actor, role) || role != GroupRoleQS::Owner) {
        return ResultQS::fail("只有群主可以切换群管理模式");
    }
    if (!next) {
        return ResultQS::fail("未知的群管理模式");
    }
    if (next->kind() == policy_->kind()) {
        return ResultQS::fail("当前已经是" + next->displayName());
    }
    std::string effect = "已切换为" + next->displayName() + "，" + std::to_string(members_.size()) + "名成员全部保留";
    const int admins = adminCount();
    if (admins > 0 && !next->supportsAdmins()) {
        effect += "；" + std::to_string(admins) + "名管理员的身份已保留，但暂按普通成员对待";
    } else if (admins > 0 && !policy_->supportsAdmins()) {
        effect += "；" + std::to_string(admins) + "名管理员恢复管理权限";
    }
    if (!subgroups_.empty()) {
        effect += next->supportsSubgroups() ? "；讨论组恢复可用" : "；" + std::to_string(subgroups_.size()) + "个讨论组转为休眠";
    }
    if (!applications_.empty() && next->joinMode() != JoinModeQS::Application) {
        effect += "；" + std::to_string(applications_.size()) + "条入群申请暂停处理（切回后可继续审批）";
    }
    policy_ = std::move(next);
    return ResultQS::ok(effect);
}

void GroupQS::addMemberDirect(const std::string& account, GroupRoleQS role, const std::string& date) {
    if (!hasMember(account)) {
        members_.emplace_back(account, role, date);
    }
}

void GroupQS::removeEverywhere(const std::string& account) {
    members_.erase(std::remove_if(members_.begin(), members_.end(),
                                  [&](const GroupMemberQS& m) { return m.account() == account; }),
                   members_.end());
    applications_.erase(std::remove_if(applications_.begin(), applications_.end(),
                                       [&](const GroupApplicationQS& a) { return a.account == account; }),
                        applications_.end());
    invitations_.erase(std::remove_if(invitations_.begin(), invitations_.end(),
                                      [&](const GroupInvitationQS& i) { return i.invitee == account; }),
                       invitations_.end());
    for (auto& s : subgroups_) {
        s.members.erase(std::remove(s.members.begin(), s.members.end(), account), s.members.end());
    }
}

void GroupQS::ensureOwner() {
    if (!members_.empty() && owner().empty()) {
        members_.front().setRole(GroupRoleQS::Owner);
    }
}

JsonQS GroupQS::toJson() const {
    JsonQS members = JsonQS::array();
    for (const auto& m : members_) members.push_back(m.toJson());
    JsonQS apps = JsonQS::array();
    for (const auto& a : applications_) apps.push_back(a.toJson());
    JsonQS invites = JsonQS::array();
    for (const auto& i : invitations_) invites.push_back(i.toJson());
    JsonQS subs = JsonQS::array();
    for (const auto& s : subgroups_) subs.push_back(s.toJson());
    return JsonQS{{"id", id_},
                  {"name", name_},
                  {"announcement", announcement_},
                  {"createdOn", createdOn_},
                  {"policy", policy_->kind()},
                  {"members", members},
                  {"applications", apps},
                  {"invitations", invites},
                  {"subgroups", subs},
                  {"nextSubgroup", nextSubgroup_}};
}

GroupQS GroupQS::fromJson(const JsonQS& j) {
    GroupQS g;
    g.id_ = j.value("id", std::string());
    g.name_ = j.value("name", std::string());
    g.announcement_ = j.value("announcement", std::string());
    g.createdOn_ = j.value("createdOn", std::string());
    auto policy = GroupPolicyRegistryQS::instance().create(j.value("policy", std::string("qq")));
    if (policy) {
        g.policy_ = std::move(policy);
    }
    for (const auto& m : j.value("members", JsonQS::array())) g.members_.push_back(GroupMemberQS::fromJson(m));
    for (const auto& a : j.value("applications", JsonQS::array())) g.applications_.push_back(GroupApplicationQS::fromJson(a));
    for (const auto& i : j.value("invitations", JsonQS::array())) g.invitations_.push_back(GroupInvitationQS::fromJson(i));
    for (const auto& s : j.value("subgroups", JsonQS::array())) g.subgroups_.push_back(SubgroupQS::fromJson(s));
    g.nextSubgroup_ = j.value("nextSubgroup", 1);
    return g;
}

}  // namespace qs

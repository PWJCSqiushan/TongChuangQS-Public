#pragma once

#include "GroupMemberQS.h"
#include "GroupPolicyQS.h"
#include "ResultQS.h"

#include <memory>
#include <string>
#include <vector>

namespace qs {

/**
 * 一个群：只保存"数据"（成员、申请、邀请、讨论组、公告），
 * 一切权限判断都委托给持有的 GroupPolicyQS 对象（运行时多态）。
 * 切换管理模式时只替换策略对象，成员与历史数据原样保留。
 */
class GroupQS {
public:
    GroupQS();
    GroupQS(std::string id, std::string name, const std::string& owner, std::unique_ptr<GroupPolicyQS> policy);
    GroupQS(const GroupQS& other);
    GroupQS& operator=(const GroupQS& other);
    GroupQS(GroupQS&&) noexcept = default;
    GroupQS& operator=(GroupQS&&) noexcept = default;
    ~GroupQS() = default;

    const std::string& id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    const std::string& announcement() const noexcept { return announcement_; }
    const std::string& createdOn() const noexcept { return createdOn_; }
    const GroupPolicyQS& policy() const { return *policy_; }
    std::string owner() const;

    const std::vector<GroupMemberQS>& members() const noexcept { return members_; }
    const std::vector<GroupApplicationQS>& applications() const noexcept { return applications_; }
    const std::vector<GroupInvitationQS>& invitations() const noexcept { return invitations_; }
    const std::vector<SubgroupQS>& subgroups() const noexcept { return subgroups_; }

    bool hasMember(const std::string& account) const;
    /** 当前管理模式下生效的角色；非成员返回 false。 */
    bool effectiveRole(const std::string& account, GroupRoleQS& role) const;
    bool hasApplication(const std::string& account) const;
    bool hasInvitation(const std::string& account) const;
    int adminCount() const;

    // —— 以下操作都会先询问策略对象是否允许 ——
    /** eligible：服务提供的额外条件（如微博需要关注群主）。 */
    ResultQS requestJoin(const std::string& account, const std::string& note, bool eligible);
    ResultQS reviewApplication(const std::string& actor, const std::string& applicant, bool accept);
    ResultQS invite(const std::string& actor, const std::string& invitee);
    ResultQS answerInvitation(const std::string& invitee, bool accept);
    ResultQS leave(const std::string& account);
    ResultQS kick(const std::string& actor, const std::string& target);
    ResultQS setAdmin(const std::string& actor, const std::string& target, bool admin);
    ResultQS transferOwner(const std::string& actor, const std::string& target);
    ResultQS createSubgroup(const std::string& actor, const std::string& name, std::vector<std::string> members);
    ResultQS setAnnouncement(const std::string& actor, const std::string& text);
    ResultQS rename(const std::string& actor, const std::string& name);

    /** 动态切换管理模式。返回值的 message 描述切换带来的影响。 */
    ResultQS switchPolicy(const std::string& actor, std::unique_ptr<GroupPolicyQS> next);

    /** 直接加成员（建群、种子数据用），不经过策略。 */
    void addMemberDirect(const std::string& account, GroupRoleQS role, const std::string& date);
    void removeEverywhere(const std::string& account);
    /** 群主账号被注销后，由最早入群的成员接任群主。 */
    void ensureOwner();

    JsonQS toJson() const;
    static GroupQS fromJson(const JsonQS& j);

private:
    GroupMemberQS* findMember(const std::string& account);
    bool roleOf(const std::string& account, GroupRoleQS& role) const;

    std::string id_;
    std::string name_;
    std::string announcement_;
    std::string createdOn_;
    std::unique_ptr<GroupPolicyQS> policy_;
    std::vector<GroupMemberQS> members_;
    std::vector<GroupApplicationQS> applications_;
    std::vector<GroupInvitationQS> invitations_;
    std::vector<SubgroupQS> subgroups_;
    int nextSubgroup_ = 1;
};

}  // namespace qs

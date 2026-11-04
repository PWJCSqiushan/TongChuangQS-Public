#pragma once

#include "JsonQS.h"

#include <string>
#include <vector>


namespace qs {

/** 群内角色。实际"生效角色"还要看当前群管理模式（见 GroupPolicyQS::effectiveRole）。 */
enum class GroupRoleQS { Owner, Admin, Member };

std::string roleName(GroupRoleQS role);
GroupRoleQS roleFromText(const std::string& text);

/** 一个群成员：账号、保存的角色、入群日期。 */
class GroupMemberQS {
public:
    GroupMemberQS() = default;
    GroupMemberQS(std::string account, GroupRoleQS role, std::string joinedOn)
        : account_(std::move(account)), role_(role), joinedOn_(std::move(joinedOn)) {}

    const std::string& account() const noexcept { return account_; }
    GroupRoleQS role() const noexcept { return role_; }
    void setRole(GroupRoleQS role) noexcept { role_ = role; }
    const std::string& joinedOn() const noexcept { return joinedOn_; }

    JsonQS toJson() const;
    static GroupMemberQS fromJson(const JsonQS& j);

private:
    std::string account_;
    GroupRoleQS role_ = GroupRoleQS::Member;
    std::string joinedOn_;
};

/** 入群申请（QQ 群特有的加入方式）。 */


/** 入群邀请（微信群唯一的加入方式；QQ 群也可邀请）。 */


/** 临时讨论组（子群），仅在支持子群的管理模式下可用。 */


}  // namespace qs

#include "GroupApplicationQS.h"
#include "GroupInvitationQS.h"
#include "SubgroupQS.h"

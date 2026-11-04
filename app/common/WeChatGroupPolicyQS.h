#pragma once

#include "GroupPolicyQS.h"

namespace qs {

/** 微信群模式：只能由群成员邀请加入；仅群主为特权账号；不允许子群。 */
class WeChatGroupPolicyQS final : public GroupPolicyQS {
public:
    std::string kind() const override { return "wechat"; }
    std::string displayName() const override { return "微信群模式"; }
    std::vector<std::string> highlights() const override;
    JoinModeQS joinMode() const override { return JoinModeQS::InviteOnly; }
    bool supportsAdmins() const override { return false; }
    bool supportsSubgroups() const override { return false; }
    bool canApprove(GroupRoleQS) const override { return false; }
    bool canInvite(GroupRoleQS) const override { return true; }
    bool canKick(GroupRoleQS actor, GroupRoleQS target) const override {
        return actor == GroupRoleQS::Owner && target != GroupRoleQS::Owner;
    }
    bool canCreateSubgroup(GroupRoleQS) const override { return false; }
    bool canEditAnnouncement(GroupRoleQS actor) const override { return actor == GroupRoleQS::Owner; }
    std::string joinHint() const override { return "微信群不接受申请，只能由群成员邀请加入"; }
    std::unique_ptr<GroupPolicyQS> clone() const override { return std::make_unique<WeChatGroupPolicyQS>(*this); }
};

}  // namespace qs

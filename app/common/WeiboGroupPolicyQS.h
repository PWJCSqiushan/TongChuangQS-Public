#pragma once

#include "GroupPolicyQS.h"

namespace qs {

/** 微博粉丝群模式：关注了群主的用户可直接加入；可设少量管理员；不支持子群。 */
class WeiboGroupPolicyQS final : public GroupPolicyQS {
public:
    std::string kind() const override { return "weibo"; }
    std::string displayName() const override { return "微博粉丝群模式"; }
    std::vector<std::string> highlights() const override;
    JoinModeQS joinMode() const override { return JoinModeQS::Open; }
    bool supportsAdmins() const override { return true; }
    int maxAdmins() const override { return 3; }
    bool supportsSubgroups() const override { return false; }
    bool canApprove(GroupRoleQS) const override { return false; }
    bool canInvite(GroupRoleQS actor) const override { return actor != GroupRoleQS::Member; }
    bool canKick(GroupRoleQS actor, GroupRoleQS target) const override {
        if (target == GroupRoleQS::Owner) return false;
        return actor == GroupRoleQS::Owner || (actor == GroupRoleQS::Admin && target == GroupRoleQS::Member);
    }
    bool canCreateSubgroup(GroupRoleQS) const override { return false; }
    bool canEditAnnouncement(GroupRoleQS actor) const override { return actor != GroupRoleQS::Member; }
    std::string joinHint() const override { return "粉丝群需先关注群主才能加入"; }
    std::unique_ptr<GroupPolicyQS> clone() const override { return std::make_unique<WeiboGroupPolicyQS>(*this); }
};

}  // namespace qs

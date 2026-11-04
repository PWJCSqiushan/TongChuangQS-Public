#pragma once

#include "GroupPolicyQS.h"

namespace qs {

/** QQ 群模式：可申请加入；以群主为核心的管理员制度；允许临时讨论组（子群）。 */
class QQGroupPolicyQS final : public GroupPolicyQS {
public:
    std::string kind() const override { return "qq"; }
    std::string displayName() const override { return "QQ群模式"; }
    std::vector<std::string> highlights() const override;
    JoinModeQS joinMode() const override { return JoinModeQS::Application; }
    bool supportsAdmins() const override { return true; }
    int maxAdmins() const override { return 5; }
    bool supportsSubgroups() const override { return true; }
    bool canApprove(GroupRoleQS actor) const override { return actor != GroupRoleQS::Member; }
    bool canInvite(GroupRoleQS) const override { return true; }
    bool canKick(GroupRoleQS actor, GroupRoleQS target) const override;
    bool canCreateSubgroup(GroupRoleQS) const override { return true; }
    bool canEditAnnouncement(GroupRoleQS actor) const override { return actor != GroupRoleQS::Member; }
    std::string joinHint() const override { return "已提交入群申请，等待群主或管理员审批"; }
    std::unique_ptr<GroupPolicyQS> clone() const override { return std::make_unique<QQGroupPolicyQS>(*this); }
};

}  // namespace qs

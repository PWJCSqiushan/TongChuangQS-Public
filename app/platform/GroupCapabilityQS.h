#pragma once

#include "GroupQS.h"
#include "ResultQS.h"

#include <string>
#include <vector>

namespace qs {

/**
 * "群"能力接口。服务按需提供；具体规则由每个群自己的 GroupPolicyQS 决定。
 * 返回的 GroupQS 为副本，界面层可放心持有。
 */
class GroupCapabilityQS {
public:
    virtual ~GroupCapabilityQS() = default;

    virtual std::vector<GroupQS> groupsOf(const std::string& me) const = 0;
    /** 群目录：本服务内所有群（用于浏览、申请加入）。 */
    virtual std::vector<GroupQS> directory() const = 0;
    virtual bool findGroup(const std::string& groupId, GroupQS& out) const = 0;
    /** 待我处理的事项数量（我可审批的申请 + 收到的邀请）。 */
    virtual int pendingCount(const std::string& me) const = 0;
    /** 本服务新建群时默认使用的管理模式。 */
    virtual std::string defaultPolicy() const = 0;

    virtual ResultQS createGroup(const std::string& me, const std::string& name, std::string& newId) = 0;
    virtual ResultQS requestJoin(const std::string& me, const std::string& groupId, const std::string& note) = 0;
    virtual ResultQS reviewApplication(const std::string& me, const std::string& groupId, const std::string& applicant,
                                       bool accept) = 0;
    virtual ResultQS invite(const std::string& me, const std::string& groupId, const std::string& invitee) = 0;
    virtual ResultQS answerInvitation(const std::string& me, const std::string& groupId, bool accept) = 0;
    virtual ResultQS leave(const std::string& me, const std::string& groupId) = 0;
    virtual ResultQS kick(const std::string& me, const std::string& groupId, const std::string& target) = 0;
    virtual ResultQS setAdmin(const std::string& me, const std::string& groupId, const std::string& target, bool admin) = 0;
    virtual ResultQS transferOwner(const std::string& me, const std::string& groupId, const std::string& target) = 0;
    virtual ResultQS createSubgroup(const std::string& me, const std::string& groupId, const std::string& name,
                                    const std::vector<std::string>& members) = 0;
    virtual ResultQS setAnnouncement(const std::string& me, const std::string& groupId, const std::string& text) = 0;
    virtual ResultQS switchPolicy(const std::string& me, const std::string& groupId, const std::string& policyKind) = 0;
};

}  // namespace qs

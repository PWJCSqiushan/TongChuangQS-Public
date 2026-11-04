#pragma once

#include "GroupCapabilityQS.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace qs {

class ServiceBaseQS;

/**
 * 可复用的群能力实现：保存本服务的全部群，并把每个操作交给群自己的策略对象裁决。
 * 服务通过构造参数定制：默认管理模式、"能否直接加入"的额外条件（微博：关注群主）。
 */
class StandardGroupsQS final : public GroupCapabilityQS {
public:
    using EligibilityQS = std::function<bool(const GroupQS& group, const std::string& account)>;

    StandardGroupsQS(ServiceBaseQS& owner, std::string defaultPolicy, EligibilityQS eligibility = nullptr);

    std::vector<GroupQS> groupsOf(const std::string& me) const override;
    std::vector<GroupQS> directory() const override;
    bool findGroup(const std::string& groupId, GroupQS& out) const override;
    int pendingCount(const std::string& me) const override;
    std::string defaultPolicy() const override { return defaultPolicy_; }

    ResultQS createGroup(const std::string& me, const std::string& name, std::string& newId) override;
    ResultQS requestJoin(const std::string& me, const std::string& groupId, const std::string& note) override;
    ResultQS reviewApplication(const std::string& me, const std::string& groupId, const std::string& applicant,
                               bool accept) override;
    ResultQS invite(const std::string& me, const std::string& groupId, const std::string& invitee) override;
    ResultQS answerInvitation(const std::string& me, const std::string& groupId, bool accept) override;
    ResultQS leave(const std::string& me, const std::string& groupId) override;
    ResultQS kick(const std::string& me, const std::string& groupId, const std::string& target) override;
    ResultQS setAdmin(const std::string& me, const std::string& groupId, const std::string& target, bool admin) override;
    ResultQS transferOwner(const std::string& me, const std::string& groupId, const std::string& target) override;
    ResultQS createSubgroup(const std::string& me, const std::string& groupId, const std::string& name,
                            const std::vector<std::string>& members) override;
    ResultQS setAnnouncement(const std::string& me, const std::string& groupId, const std::string& text) override;
    ResultQS switchPolicy(const std::string& me, const std::string& groupId, const std::string& policyKind) override;

    /** 种子数据：直接建群并加入成员（members 中第一个为群主）。 */
    GroupQS& seedGroup(const std::string& id, const std::string& name, const std::string& policyKind,
                       const std::vector<std::string>& members, const std::string& announcement = "");
    void forget(const std::string& account);

    JsonQS toJson() const;
    void fromJson(const JsonQS& j);

private:
    ResultQS withGroup(const std::string& groupId, const std::string& actor, const std::function<ResultQS(GroupQS&)>& action);

    ServiceBaseQS& owner_;
    std::string defaultPolicy_;
    EligibilityQS eligibility_;
    std::map<std::string, GroupQS> groups_;
};

}  // namespace qs

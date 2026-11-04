#pragma once

#include "FriendBookQS.h"
#include "FriendCapabilityQS.h"

#include <string>

namespace qs {

class ServiceBaseQS;

/**
 * 可复用的"申请-审批式双向好友"实现。
 * QQ、微信、微商各自"组合"一个本类对象并交给它自己的 FriendBookQS，
 * 数据仍然保存在各自服务的目录下——复用的是代码，不是数据。
 */
class StandardFriendsQS final : public FriendCapabilityQS {
public:
    StandardFriendsQS(ServiceBaseQS& owner, FriendBookQS& book, std::string relationName = "好友");

    std::string relationName() const override { return relationName_; }
    std::string addActionName() const override { return "添加" + relationName_; }
    bool requiresApproval() const override { return true; }

    std::vector<FriendViewQS> friendsOf(const std::string& me) const override;
    bool areFriends(const std::string& a, const std::string& b) const override;
    ResultQS addFriend(const std::string& me, const std::string& target, const std::string& note) override;
    std::vector<FriendRequestQS> incomingRequests(const std::string& me) const override;
    std::vector<FriendRequestQS> outgoingRequests(const std::string& me) const override;
    ResultQS answerRequest(const std::string& me, const std::string& requestId, bool accept) override;
    ResultQS removeFriend(const std::string& me, const std::string& target) override;
    ResultQS updateFriend(const std::string& me, const std::string& target, const std::string& remark,
                          const std::string& tag) override;

private:
    ServiceBaseQS& owner_;
    FriendBookQS& book_;
    std::string relationName_;
};

}  // namespace qs

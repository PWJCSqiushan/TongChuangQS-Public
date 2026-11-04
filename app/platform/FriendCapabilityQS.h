#pragma once

#include "FriendRequestQS.h"
#include "ResultQS.h"

#include <string>
#include <vector>

namespace qs {

/**
 * "好友"能力接口。不是所有服务都必须有好友功能——服务若支持，
 * 就在 ServiceBaseQS::friendCapability() 中返回自己的实现。
 * QQ/微信/微商复用 StandardFriendsQS（申请-审批的双向好友），
 * 微博提供 WeiboFollowsQS（单向关注、互关即好友）——同一接口，不同行为。
 */
class FriendCapabilityQS {
public:
    virtual ~FriendCapabilityQS() = default;

    /** 关系名称，如"好友""互关好友""客户"。 */
    virtual std::string relationName() const = 0;
    /** 添加动作的名称，如"添加好友""关注"。 */
    virtual std::string addActionName() const = 0;
    /** 添加是否需要对方同意。 */
    virtual bool requiresApproval() const = 0;

    virtual std::vector<FriendViewQS> friendsOf(const std::string& me) const = 0;
    virtual bool areFriends(const std::string& a, const std::string& b) const = 0;
    virtual ResultQS addFriend(const std::string& me, const std::string& target, const std::string& note) = 0;
    virtual std::vector<FriendRequestQS> incomingRequests(const std::string& me) const = 0;
    virtual std::vector<FriendRequestQS> outgoingRequests(const std::string& me) const = 0;
    virtual ResultQS answerRequest(const std::string& me, const std::string& requestId, bool accept) = 0;
    virtual ResultQS removeFriend(const std::string& me, const std::string& target) = 0;
    virtual ResultQS updateFriend(const std::string& me, const std::string& target, const std::string& remark,
                                  const std::string& tag) = 0;
};

}  // namespace qs

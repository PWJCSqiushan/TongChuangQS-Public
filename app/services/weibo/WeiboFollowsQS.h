#pragma once

#include "FriendCapabilityQS.h"

#include <map>
#include <set>
#include <string>
#include <utility>

namespace qs {

class ServiceBaseQS;

/**
 * 微博的"好友"能力：单向关注，互相关注即为好友，无需审批。
 * 与 StandardFriendsQS 实现同一个 FriendCapabilityQS 接口，但行为完全不同——
 * 平台和通用联系人页面只认接口，不关心是哪一种实现（多态）。
 * 映射关系：  加好友 = 关注；  收到的申请 = 新粉丝（"同意"即回关）；  发出的申请 = 我关注但未回关的人。
 */
class WeiboFollowsQS final : public FriendCapabilityQS {
public:
    explicit WeiboFollowsQS(ServiceBaseQS& owner);

    std::string relationName() const override { return "互关好友"; }
    std::string addActionName() const override { return "关注"; }
    bool requiresApproval() const override { return false; }

    std::vector<FriendViewQS> friendsOf(const std::string& me) const override;
    bool areFriends(const std::string& a, const std::string& b) const override;
    ResultQS addFriend(const std::string& me, const std::string& target, const std::string& note) override;
    std::vector<FriendRequestQS> incomingRequests(const std::string& me) const override;
    std::vector<FriendRequestQS> outgoingRequests(const std::string& me) const override;
    ResultQS answerRequest(const std::string& me, const std::string& requestId, bool accept) override;
    ResultQS removeFriend(const std::string& me, const std::string& target) override;
    ResultQS updateFriend(const std::string& me, const std::string& target, const std::string& remark,
                          const std::string& tag) override;

    bool follows(const std::string& follower, const std::string& followee) const;
    std::set<std::string> followingOf(const std::string& me) const;
    int followerCount(const std::string& me) const;
    int followingCount(const std::string& me) const;

    /** 种子数据与注销清理（在服务的事务内调用）。 */
    void addFollow(const std::string& follower, const std::string& followee, const std::string& date);
    void forget(const std::string& account);

    JsonQS toJson() const;
    void fromJson(const JsonQS& j);

private:
    ServiceBaseQS& owner_;
    std::map<std::pair<std::string, std::string>, std::string> follows_;  // (follower, followee) → 日期
    std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string>> notes_;  // 备注/分组
    std::set<std::pair<std::string, std::string>> dismissed_;  // 被忽略的"新粉丝"提醒
};

}  // namespace qs

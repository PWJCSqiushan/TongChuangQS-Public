#pragma once

#include "FriendRequestQS.h"
#include "ResultQS.h"

#include <map>
#include <string>
#include <vector>

#include "FriendLinkQS.h"

namespace qs {

/** 单向的好友属性：我给对方设置的备注、分组，以及成为好友的日期。 */


/**
 * 好友容器：保存双向好友关系与待处理申请。
 * QQ、微信、微商通过组合复用它；微博的"关注"语义不同，另有实现。
 */
class FriendBookQS {
public:
    bool areFriends(const std::string& a, const std::string& b) const;
    void connect(const std::string& a, const std::string& b, const std::string& date);
    bool disconnect(const std::string& a, const std::string& b);
    std::map<std::string, FriendLinkQS> linksOf(const std::string& owner) const;
    FriendLinkQS* link(const std::string& owner, const std::string& other);

    const std::vector<FriendRequestQS>& requests() const noexcept { return requests_; }
    const FriendRequestQS* findRequest(const std::string& id) const;
    bool hasPending(const std::string& from, const std::string& to) const;
    std::string addRequest(const std::string& from, const std::string& to, const std::string& note, const std::string& date);
    void removeRequest(const std::string& id);

    /** 账号注销时清理与它有关的所有关系。 */
    void forget(const std::string& account);

    JsonQS toJson() const;
    void fromJson(const JsonQS& j);

private:
    std::map<std::string, std::map<std::string, FriendLinkQS>> links_;
    std::vector<FriendRequestQS> requests_;
    long long nextRequest_ = 1;
};

}  // namespace qs

#include "StandardFriendsQS.h"

#include "DateQS.h"
#include "ServiceBaseQS.h"

namespace qs {

StandardFriendsQS::StandardFriendsQS(ServiceBaseQS& owner, FriendBookQS& book, std::string relationName)
    : owner_(owner), book_(book), relationName_(std::move(relationName)) {}

std::vector<FriendViewQS> StandardFriendsQS::friendsOf(const std::string& me) const {
    std::vector<FriendViewQS> result;
    for (const auto& entry : book_.linksOf(me)) {
        if (owner_.account(entry.first) == nullptr) continue;
        result.push_back(FriendViewQS{entry.first, owner_.nicknameOf(entry.first), entry.second.remark, entry.second.tag,
                                      entry.second.since, relationName_});
    }
    return result;
}

bool StandardFriendsQS::areFriends(const std::string& a, const std::string& b) const { return book_.areFriends(a, b); }

ResultQS StandardFriendsQS::addFriend(const std::string& me, const std::string& target, const std::string& note) {
    return owner_.transact([&]() -> ResultQS {
        if (owner_.account(me) == nullptr) return ResultQS::fail("当前账号不存在，请重新登录");
        if (owner_.account(target) == nullptr) {
            return ResultQS::fail("没有找到" + owner_.accountLabel() + " " + target);
        }
        if (target == me) return ResultQS::fail("不能添加自己");
        if (book_.areFriends(me, target)) return ResultQS::fail("你们已经是" + relationName_);
        if (book_.hasPending(me, target)) return ResultQS::fail("已发送过申请，请等待对方处理");
        for (const auto& r : book_.requests()) {
            if (r.from == target && r.to == me) {  // 对方也申请了我：直接通过
                book_.removeRequest(r.id);
                book_.connect(me, target, DateQS::today());
                return ResultQS::ok("对方也向你发过申请，你们已成为" + relationName_);
            }
        }
        book_.addRequest(me, target, note, DateQS::today());
        return ResultQS::ok("申请已发送，等待 " + owner_.nicknameOf(target) + " 同意");
    });
}

std::vector<FriendRequestQS> StandardFriendsQS::incomingRequests(const std::string& me) const {
    std::vector<FriendRequestQS> result;
    for (const auto& r : book_.requests()) {
        if (r.to == me) result.push_back(r);
    }
    return result;
}

std::vector<FriendRequestQS> StandardFriendsQS::outgoingRequests(const std::string& me) const {
    std::vector<FriendRequestQS> result;
    for (const auto& r : book_.requests()) {
        if (r.from == me) result.push_back(r);
    }
    return result;
}

ResultQS StandardFriendsQS::answerRequest(const std::string& me, const std::string& requestId, bool accept) {
    return owner_.transact([&]() -> ResultQS {
        if (owner_.account(me) == nullptr) return ResultQS::fail("当前账号不存在，请重新登录");
        const FriendRequestQS* r = book_.findRequest(requestId);
        if (r == nullptr || r->to != me) return ResultQS::fail("申请不存在或已处理");
        const std::string from = r->from;
        book_.removeRequest(requestId);
        if (!accept) return ResultQS::ok("已拒绝");
        if (owner_.account(from) == nullptr) return ResultQS::fail("对方账号已注销");
        book_.connect(me, from, DateQS::today());
        return ResultQS::ok("你和 " + owner_.nicknameOf(from) + " 已成为" + relationName_);
    });
}

ResultQS StandardFriendsQS::removeFriend(const std::string& me, const std::string& target) {
    return owner_.transact([&]() -> ResultQS {
        if (owner_.account(me) == nullptr) return ResultQS::fail("当前账号不存在，请重新登录");
        if (!book_.disconnect(me, target)) return ResultQS::fail("对方不是你的" + relationName_);
        return ResultQS::ok("已删除" + relationName_ + " " + owner_.nicknameOf(target));
    });
}

ResultQS StandardFriendsQS::updateFriend(const std::string& me, const std::string& target, const std::string& remark,
                                         const std::string& tag) {
    return owner_.transact([&]() -> ResultQS {
        if (owner_.account(me) == nullptr) return ResultQS::fail("当前账号不存在，请重新登录");
        FriendLinkQS* link = book_.link(me, target);
        if (link == nullptr) return ResultQS::fail("对方不是你的" + relationName_);
        link->remark = remark;
        link->tag = tag;
        return ResultQS::ok("备注与分组已保存");
    });
}

}  // namespace qs

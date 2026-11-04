#include "WeiboFollowsQS.h"

#include "DateQS.h"
#include "ServiceBaseQS.h"

namespace qs {

WeiboFollowsQS::WeiboFollowsQS(ServiceBaseQS& owner) : owner_(owner) {}

bool WeiboFollowsQS::follows(const std::string& follower, const std::string& followee) const {
    return follows_.count({follower, followee}) != 0;
}

bool WeiboFollowsQS::areFriends(const std::string& a, const std::string& b) const { return follows(a, b) && follows(b, a); }

std::vector<FriendViewQS> WeiboFollowsQS::friendsOf(const std::string& me) const {
    std::vector<FriendViewQS> result;
    for (const auto& f : follows_) {
        if (f.first.first != me || !follows(f.first.second, me)) continue;
        const std::string& other = f.first.second;
        if (owner_.account(other) == nullptr) continue;
        FriendViewQS view{other, owner_.nicknameOf(other), "", "", f.second, "互相关注"};
        const auto note = notes_.find({me, other});
        if (note != notes_.end()) {
            view.remark = note->second.first;
            view.tag = note->second.second;
        }
        result.push_back(view);
    }
    return result;
}

ResultQS WeiboFollowsQS::addFriend(const std::string& me, const std::string& target, const std::string&) {
    return owner_.transact([&]() -> ResultQS {
        if (!owner_.account(me)) return ResultQS::fail("当前账号不存在，请重新登录");
        if (owner_.account(target) == nullptr) return ResultQS::fail("没有找到微博用户 " + target);
        if (target == me) return ResultQS::fail("不能关注自己");
        if (follows(me, target)) return ResultQS::fail("你已经关注了对方");
        follows_[{me, target}] = DateQS::today();
        dismissed_.erase({target, me});
        return ResultQS::ok(follows(target, me) ? "已回关，你们成为互关好友" : "已关注，对方回关后即成为互关好友");
    });
}

std::vector<FriendRequestQS> WeiboFollowsQS::incomingRequests(const std::string& me) const {
    std::vector<FriendRequestQS> result;
    for (const auto& f : follows_) {
        const std::string& fan = f.first.first;
        if (f.first.second == me && !follows(me, fan) && !dismissed_.count({me, fan})) {
            result.push_back(FriendRequestQS{"fan:" + fan, fan, me, "关注了你（回关即成为互关好友）", f.second});
        }
    }
    return result;
}

std::vector<FriendRequestQS> WeiboFollowsQS::outgoingRequests(const std::string& me) const {
    std::vector<FriendRequestQS> result;
    for (const auto& f : follows_) {
        if (f.first.first == me && !follows(f.first.second, me)) {
            result.push_back(FriendRequestQS{"follow:" + f.first.second, me, f.first.second, "已关注，等待对方回关", f.second});
        }
    }
    return result;
}

ResultQS WeiboFollowsQS::answerRequest(const std::string& me, const std::string& requestId, bool accept) {
    if (requestId.rfind("fan:", 0) != 0) return ResultQS::fail("无效的提醒");
    const std::string fan = requestId.substr(4);
    if (accept) return addFriend(me, fan, "");
    return owner_.transact([&]() -> ResultQS {
        if (!owner_.account(me)) return ResultQS::fail("当前账号不存在，请重新登录");
        dismissed_.insert({me, fan});
        return ResultQS::ok("已忽略");
    });
}

ResultQS WeiboFollowsQS::removeFriend(const std::string& me, const std::string& target) {
    return owner_.transact([&]() -> ResultQS {
        if (!owner_.account(me)) return ResultQS::fail("当前账号不存在，请重新登录");
        if (follows_.erase({me, target}) == 0) return ResultQS::fail("你没有关注对方");
        return ResultQS::ok("已取消关注");
    });
}

ResultQS WeiboFollowsQS::updateFriend(const std::string& me, const std::string& target, const std::string& remark,
                                      const std::string& tag) {
    return owner_.transact([&]() -> ResultQS {
        if (!owner_.account(me)) return ResultQS::fail("当前账号不存在，请重新登录");
        if (!follows(me, target)) return ResultQS::fail("请先关注对方");
        notes_[{me, target}] = {remark, tag};
        return ResultQS::ok("备注与分组已保存");
    });
}

std::set<std::string> WeiboFollowsQS::followingOf(const std::string& me) const {
    std::set<std::string> result;
    for (const auto& f : follows_) {
        if (f.first.first == me) result.insert(f.first.second);
    }
    return result;
}

int WeiboFollowsQS::followerCount(const std::string& me) const {
    int n = 0;
    for (const auto& f : follows_) n += f.first.second == me ? 1 : 0;
    return n;
}

int WeiboFollowsQS::followingCount(const std::string& me) const { return static_cast<int>(followingOf(me).size()); }

void WeiboFollowsQS::addFollow(const std::string& follower, const std::string& followee, const std::string& date) {
    follows_[{follower, followee}] = date;
}

void WeiboFollowsQS::forget(const std::string& account) {
    for (auto it = follows_.begin(); it != follows_.end();) {
        it = (it->first.first == account || it->first.second == account) ? follows_.erase(it) : std::next(it);
    }
}

JsonQS WeiboFollowsQS::toJson() const {
    JsonQS list = JsonQS::array();
    for (const auto& f : follows_) {
        JsonQS item{{"from", f.first.first}, {"to", f.first.second}, {"since", f.second}};
        const auto note = notes_.find(f.first);
        if (note != notes_.end()) {
            item["remark"] = note->second.first;
            item["tag"] = note->second.second;
        }
        list.push_back(item);
    }
    JsonQS dismissed = JsonQS::array();
    for (const auto& d : dismissed_) dismissed.push_back(JsonQS{d.first, d.second});
    return JsonQS{{"follows", list}, {"dismissed", dismissed}};
}

void WeiboFollowsQS::fromJson(const JsonQS& j) {
    follows_.clear();
    notes_.clear();
    dismissed_.clear();
    for (const auto& item : j.value("follows", JsonQS::array())) {
        const std::pair<std::string, std::string> key{item.value("from", std::string()), item.value("to", std::string())};
        follows_[key] = item.value("since", std::string());
        if (item.contains("remark")) notes_[key] = {item.value("remark", std::string()), item.value("tag", std::string())};
    }
    for (const auto& d : j.value("dismissed", JsonQS::array())) {
        if (d.is_array() && d.size() == 2) dismissed_.insert({d[0].get<std::string>(), d[1].get<std::string>()});
    }
}

}  // namespace qs

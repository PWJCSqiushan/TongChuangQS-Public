#include "WeiboServiceQS.h"

#include "DateQS.h"
#include "ServiceRegistryQS.h"

#include <algorithm>

namespace qs {

WeiboServiceQS::WeiboServiceQS()
    : ServiceBaseQS("weibo", "微博"),
      follows_(*this),
      // 粉丝群的入群条件：关注了群主
      groups_(*this, "weibo", [this](const GroupQS& g, const std::string& who) { return follows_.follows(who, g.owner()); }) {}

std::vector<ServiceFeatureQS> WeiboServiceQS::features() const {
    return {{"weibo.feed", "首页", "home"}, {"contacts", "关注", "contacts"}, {"groups", "粉丝群", "group"}, {"me", "我", "me"}};
}

std::vector<RegistrationFieldQS> WeiboServiceQS::idFields() const {
    return {{"qq", "QQ号", "微博与QQ共用号码，填写已有QQ号", false, false},
            {"qqPassword", "QQ密码", "用于向QQ服务核验身份", true, false}};
}

ResultQS WeiboServiceQS::prepareAccountId(const FormQS& form, std::string& id) {
    const auto qq = form.find("qq");
    const auto pwd = form.find("qqPassword");
    if (qq == form.end() || qq->second.empty()) return ResultQS::fail("请填写QQ号");
    reloadIfChanged();
    if (accounts().contains(qq->second)) return ResultQS::fail("该号码已开通微博，请直接登录");
    // 号码属于 QQ 服务：经总线请 QQ 服务核验，微博看不到 QQ 的账号数据。
    const ServiceReplyQS reply = bus()->send(ServiceRequestQS{
        ServiceBaseQS::id(), "qq", "verify_credentials",
        JsonQS{{"account", qq->second}, {"password", pwd == form.end() ? std::string() : pwd->second}}});
    if (!reply.ok) return ResultQS::fail("QQ号核验失败：" + reply.message);
    id = qq->second;
    return ResultQS::ok();
}

void WeiboServiceQS::onAccountCreated(ServiceAccountQS& account, const FormQS&) {
    account.addLink(AccountRefQS("qq", account.id()));
}

ResultQS WeiboServiceQS::createLinkedAccount(const UserProfileQS& profile, const std::string& password,
                                             const AccountRefQS& source, std::string& newId) {
    if (source.service() != "qq") return ResultQS::fail("微博只与QQ共用号码");
    if (accounts().contains(source.account())) return ResultQS::fail("该号码已开通微博");
    newId = source.account();
    ServiceAccountQS account(newId, password, UserProfileQS(profile.nickname(), profile.birthday(), profile.location(),
                                                            DateQS::today()));
    account.addLink(source);
    const ResultQS added = mutableAccounts().add(std::move(account));
    return added ? ResultQS::ok("已同时开通微博，微博号与QQ号相同：" + newId) : added;
}

void WeiboServiceQS::onAccountRemoved(const std::string& id) {
    follows_.forget(id);
    groups_.forget(id);
    posts_.erase(std::remove_if(posts_.begin(), posts_.end(), [&](const WeiboPostQS& p) { return p.author == id; }),
                 posts_.end());
}

ResultQS WeiboServiceQS::post(const std::string& me, const std::string& text) {
    if (text.empty()) return ResultQS::fail("说点什么吧");
    if (text.size() > 420) return ResultQS::fail("微博最多140字");
    return transact([&]() -> ResultQS {
        posts_.push_back(WeiboPostQS{"p" + std::to_string(nextPost_++), me, text, DateQS::nowMillis(), {}});
        return ResultQS::ok("发布成功");
    });
}

ResultQS WeiboServiceQS::toggleLike(const std::string& me, const std::string& postId) {
    return transact([&]() -> ResultQS {
        for (auto& p : posts_) {
            if (p.id != postId) continue;
            if (p.likes.erase(me) == 0) {
                p.likes.insert(me);
                return ResultQS::ok("已点赞");
            }
            return ResultQS::ok("已取消点赞");
        }
        return ResultQS::fail("微博不存在");
    });
}

ResultQS WeiboServiceQS::deletePost(const std::string& me, const std::string& postId) {
    return transact([&]() -> ResultQS {
        const auto it = std::find_if(posts_.begin(), posts_.end(), [&](const WeiboPostQS& p) { return p.id == postId; });
        if (it == posts_.end()) return ResultQS::fail("微博不存在");
        if (it->author != me) return ResultQS::fail("只能删除自己的微博");
        posts_.erase(it);
        return ResultQS::ok("已删除");
    });
}

std::vector<WeiboPostQS> WeiboServiceQS::timeline(const std::string& me) const {
    const std::set<std::string> following = follows_.followingOf(me);
    std::vector<WeiboPostQS> result;
    for (const auto& p : posts_) {
        if (p.author == me || following.count(p.author)) result.push_back(p);
    }
    std::sort(result.begin(), result.end(), [](const WeiboPostQS& a, const WeiboPostQS& b) { return a.time > b.time; });
    return result;
}

std::vector<WeiboPostQS> WeiboServiceQS::postsOf(const std::string& author) const {
    std::vector<WeiboPostQS> result;
    for (const auto& p : posts_) {
        if (p.author == author) result.push_back(p);
    }
    std::sort(result.begin(), result.end(), [](const WeiboPostQS& a, const WeiboPostQS& b) { return a.time > b.time; });
    return result;
}

void WeiboServiceQS::loadData() {
    follows_.fromJson(readDataFile("follows.json"));
    groups_.fromJson(readDataFile("groups.json", JsonQS::array()));
    const JsonQS posts = readDataFile("posts.json");
    posts_.clear();
    for (const auto& p : posts.value("posts", JsonQS::array())) posts_.push_back(WeiboPostQS::fromJson(p));
    nextPost_ = posts.value("next", 1LL);
}

void WeiboServiceQS::saveData() const {
    writeDataFile("follows.json", follows_.toJson());
    writeDataFile("groups.json", groups_.toJson());
    JsonQS list = JsonQS::array();
    for (const auto& p : posts_) list.push_back(p.toJson());
    writeDataFile("posts.json", JsonQS{{"posts", list}, {"next", nextPost_}});
}

void WeiboServiceQS::seedDemoData() {
    seedAccount("10001", "丘山", "2006-03-18", "吉林长春", "2019-05-01", {{"qq", "10001"}}, "记录生活");
    seedAccount("10002", "林然", "2006-08-09", "吉林长春", "2019-06-01", {{"qq", "10002"}});
    seedAccount("10003", "许知", "2005-11-23", "辽宁沈阳", "2018-01-01", {{"qq", "10003"}});
    seedAccount("10005", "陈序", "2006-05-05", "吉林吉林", "2020-03-01", {{"qq", "10005"}});
    seedAccount("10007", "沈禾", "2006-12-12", "浙江杭州", "2021-04-01", {{"qq", "10007"}});
    seedAccount("10008", "顾言", "2006-04-04", "江苏南京", "2022-05-01", {{"qq", "10008"}});

    const std::pair<const char*, const char*> follows[] = {{"10001", "10002"}, {"10002", "10001"}, {"10001", "10003"},
                                                          {"10003", "10001"}, {"10001", "10008"}, {"10005", "10001"},
                                                          {"10007", "10001"}, {"10002", "10007"}, {"10008", "10003"}};
    for (const auto& f : follows) follows_.addFollow(f.first, f.second, "2025-11-01");

    const std::int64_t now = DateQS::nowMillis();
    const std::int64_t hour = 3600LL * 1000;
    posts_.push_back(WeiboPostQS{"p1", "10002", "周六早上跑了10公里，配速5分30，舒服！", now - 30 * hour, {"10001"}});
    posts_.push_back(WeiboPostQS{"p2", "10003", "数学竞赛还有一个月，今天把多元积分又过了一遍。", now - 20 * hour, {}});
    posts_.push_back(WeiboPostQS{"p3", "10001", "C++ 课设终于把服务插件化跑通了：新增一个服务只需要写它自己的代码。", now - 6 * hour, {"10002", "10003"}});
    posts_.push_back(WeiboPostQS{"p4", "10008", "南京今天下雨了，适合在宿舍写代码。", now - 3 * hour, {}});
    posts_.push_back(WeiboPostQS{"p5", "10005", "求推荐好用的 C++ 教材～", now - 1 * hour, {"10001"}});
    nextPost_ = 6;

    groups_.seedGroup("1001", "丘山的粉丝群", "weibo", {"10001", "10002", "10003", "10005"}, "欢迎来聊课设和跑步");
    groups_.seedGroup("1002", "跑步日常", "weibo", {"10002", "10001", "10007"});
    groups_.seedGroup("1003", "数学爱好者", "weibo", {"10003", "10008"});
    groups_.seedGroup("1004", "编程学习打卡", "weibo", {"10005", "10001"});
    groups_.seedGroup("1005", "杭州生活", "weibo", {"10007"});
    groups_.seedGroup("1006", "南京同城", "weibo", {"10008"});
}

QS_REGISTER_SERVICE(WeiboServiceQS)

}  // namespace qs

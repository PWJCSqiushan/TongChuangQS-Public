// 核心层测试：平台只通过 ServiceBaseQS 与能力接口访问服务，
// 测试也尽量只用这些抽象接口（个别服务特色功能除外）。
#include "PathQS.h"
#include "PlatformQS.h"
#include "TestHarnessQS.h"
#include "WeShopServiceQS.h"
#include "WeiboServiceQS.h"

#include <algorithm>
#include <chrono>
#include <filesystem>

using namespace qs;

namespace {

std::string freshDir(const std::string& name) {
    const auto dir = std::filesystem::temp_directory_path() /
                     ("qs-core-" + name + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    return PathQS::toUtf8(dir);
}

bool hasFriend(const std::vector<FriendViewQS>& list, const std::string& account) {
    return std::any_of(list.begin(), list.end(), [&](const FriendViewQS& f) { return f.account == account; });
}

}  // namespace

QS_TEST(ServicesAreDiscoveredByRegistry) {
    PlatformQS platform(freshDir("registry"));
    platform.start();
    QS_CHECK(platform.services().size() == 4);
    QS_CHECK(platform.services()[0]->id() == "qq");
    QS_CHECK(platform.services()[3]->id() == "weshop");
    for (const auto& s : platform.services()) {
        // 题目要求：每个微X都预设 1001-1006 号群
        GroupQS g;
        for (int id = 1001; id <= 1006; ++id) QS_CHECK_MSG(s->groupCapability()->findGroup(std::to_string(id), g), s->id());
    }
}

QS_TEST(LoginMakesLinkedServicesPending) {
    PlatformQS platform(freshDir("login"));
    platform.start();
    QS_CHECK(!platform.login("qq", "10001", "wrong"));
    QS_CHECK(platform.login("qq", "10001", "123456"));
    QS_CHECK(platform.session().isLoggedIn("qq"));
    QS_CHECK(platform.session().isPending("wechat"));
    QS_CHECK(platform.session().accountOf("wechat") == "qiushan");
    QS_CHECK(platform.session().accountOf("weibo") == "10001");
    QS_CHECK(platform.session().accountOf("weshop") == "qiushanshop");  // 微商→微信→QQ 间接关联
    QS_CHECK(platform.confirmPending("wechat"));
    QS_CHECK(platform.session().isLoggedIn("wechat"));
    platform.logout("qq");
    QS_CHECK(!platform.session().isLoggedIn("qq"));
    QS_CHECK(platform.session().isLoggedIn("wechat"));
}

QS_TEST(LinkedRegistrationStoresDataSeparately) {
    PlatformQS platform(freshDir("register"));
    platform.start();
    ServiceBaseQS* qq = platform.service("qq");
    std::string id;
    const FormQS form{{"nickname", "测试员"}, {"password", "abcdef"}, {"birthday", "2006-01-01"}, {"location", "长春"}};
    QS_CHECK(qq->registerAccount(form, id));
    QS_CHECK(id == "10009");
    const auto targets = platform.linkedRegistrationTargets("qq");
    QS_CHECK(std::find(targets.begin(), targets.end(), "wechat") != targets.end());
    QS_CHECK(std::find(targets.begin(), targets.end(), "weibo") != targets.end());
    const auto results = platform.registerLinked(AccountRefQS("qq", id), "abcdef", {"wechat", "weibo"});
    QS_CHECK(results.size() == 2 && results[0].second && results[1].second);
    QS_CHECK(platform.service("weibo")->account(id) != nullptr);
    const std::string wx = platform.linkedAccountIn(AccountRefQS("qq", id), "wechat");
    QS_CHECK(wx.rfind("wxid_", 0) == 0);
    // 单独注销微信，QQ 与微博不受影响
    QS_CHECK(platform.service("wechat")->deactivate(wx, "abcdef"));
    QS_CHECK(platform.service("wechat")->account(wx) == nullptr);
    QS_CHECK(qq->account(id) != nullptr);
    QS_CHECK(platform.service("weibo")->account(id) != nullptr);
    // 校验失败的注册
    QS_CHECK(!qq->registerAccount(FormQS{{"nickname", ""}, {"password", "abcdef"}, {"birthday", "2006-01-01"}, {"location", "x"}}, id));
    QS_CHECK(!platform.service("wechat")->registerAccount(
        FormQS{{"wxid", "1bad"}, {"nickname", "a"}, {"password", "abcdef"}, {"birthday", "2006-01-01"}, {"location", "x"}}, id));
}

QS_TEST(WeiboRegistrationVerifiesQQThroughBus) {
    PlatformQS platform(freshDir("weibo-reg"));
    platform.start();
    ServiceBaseQS* weibo = platform.service("weibo");
    std::string id;
    FormQS form{{"qq", "10004"}, {"qqPassword", "bad"}, {"nickname", "江屿"}, {"password", "123456"},
                {"birthday", "2006-01-30"}, {"location", "哈尔滨"}};
    QS_CHECK(!weibo->registerAccount(form, id));
    form["qqPassword"] = "123456";
    QS_CHECK(weibo->registerAccount(form, id));
    QS_CHECK(id == "10004");
    const auto& log = platform.busLog();
    QS_CHECK(std::any_of(log.begin(), log.end(), [](const BusLogEntryQS& e) {
        return e.from == "weibo" && e.to == "qq" && e.kind == "verify_credentials" && e.ok;
    }));
}

QS_TEST(CrossServiceRecommendationAndCommonFriends) {
    PlatformQS platform(freshDir("recommend"));
    platform.start();
    std::string hint;
    const auto candidates = platform.recommendFriends("wechat", "qiushan", "qq", hint);
    std::vector<std::string> ids;
    for (const auto& c : candidates) ids.push_back(c.account);
    QS_CHECK_MSG(std::find(ids.begin(), ids.end(), "xuzhi") != ids.end(), hint);
    QS_CHECK(std::find(ids.begin(), ids.end(), "zhouning") != ids.end());
    QS_CHECK(std::find(ids.begin(), ids.end(), "linran") == ids.end());  // 已是好友
    const auto common = platform.commonFriends(AccountRefQS("qq", "10001"), "wechat", hint);
    QS_CHECK_MSG(common.size() == 2, hint);
}

QS_TEST(FriendRequestFlow) {
    PlatformQS platform(freshDir("friends"));
    platform.start();
    FriendCapabilityQS* f = platform.service("qq")->friendCapability();
    QS_CHECK(f->incomingRequests("10001").size() == 1);
    const std::string requestId = f->incomingRequests("10001")[0].id;
    QS_CHECK(f->answerRequest("10001", requestId, true));
    QS_CHECK(f->areFriends("10001", "10008"));
    QS_CHECK(f->addFriend("10007", "10001", "hi"));
    QS_CHECK(!f->addFriend("10007", "10001", "again"));
    QS_CHECK(f->addFriend("10001", "10007", ""));  // 对方已申请 → 直接成为好友
    QS_CHECK(f->areFriends("10001", "10007"));
    QS_CHECK(f->updateFriend("10001", "10007", "小沈", "同学"));
    QS_CHECK(f->friendsOf("10001").size() == 7);
    QS_CHECK(f->removeFriend("10001", "10007"));
    QS_CHECK(!hasFriend(f->friendsOf("10001"), "10007"));
}

QS_TEST(GroupPolicySwitchKeepsMembers) {
    PlatformQS platform(freshDir("groups"));
    platform.start();
    GroupCapabilityQS* g = platform.service("qq")->groupCapability();
    QS_CHECK(g->requestJoin("10004", "1001", "我也想加入"));
    GroupQS group;
    g->findGroup("1001", group);
    QS_CHECK(group.applications().size() == 2);
    const std::size_t members = group.members().size();
    QS_CHECK(g->kick("10002", "1001", "10005"));  // QQ模式：管理员可移出成员
    QS_CHECK(g->switchPolicy("10001", "1001", "wechat"));
    g->findGroup("1001", group);
    QS_CHECK(group.members().size() == members - 1);
    GroupRoleQS role = GroupRoleQS::Owner;
    QS_CHECK(group.effectiveRole("10002", role) && role == GroupRoleQS::Member);  // 管理员暂按成员对待
    QS_CHECK(!g->kick("10002", "1001", "10003"));         // 微信模式：只有群主能踢人
    QS_CHECK(!g->requestJoin("10008", "1001", ""));       // 微信模式：不能申请
    QS_CHECK(!g->createSubgroup("10001", "1001", "x", {"10003"}));
    QS_CHECK(g->invite("10003", "1001", "10004"));        // 微信模式：成员可邀请
    QS_CHECK(g->answerInvitation("10004", "1001", true));
    QS_CHECK(g->switchPolicy("10001", "1001", "qq"));
    g->findGroup("1001", group);
    QS_CHECK(group.effectiveRole("10002", role) && role == GroupRoleQS::Admin);  // 切回后恢复
    QS_CHECK(group.subgroups().size() == 1);
    QS_CHECK(g->reviewApplication("10002", "1001", "10008", true));
    QS_CHECK(!g->leave("10001", "1001"));  // 群主需先转让
    QS_CHECK(g->transferOwner("10001", "1001", "10002"));
    QS_CHECK(g->leave("10001", "1001"));
}

QS_TEST(WeiboFollowSemantics) {
    PlatformQS platform(freshDir("weibo"));
    platform.start();
    auto* weibo = dynamic_cast<WeiboServiceQS*>(platform.service("weibo"));
    FriendCapabilityQS* f = weibo->friendCapability();
    QS_CHECK(!f->requiresApproval());
    QS_CHECK(f->areFriends("10001", "10002"));
    QS_CHECK(!f->incomingRequests("10001").empty());  // 新粉丝 10005、10007
    QS_CHECK(f->answerRequest("10001", "fan:10005", true));  // 回关
    QS_CHECK(f->areFriends("10001", "10005"));
    GroupCapabilityQS* g = weibo->groupCapability();
    QS_CHECK(!g->requestJoin("10007", "1003", ""));  // 未关注群主 10003
    QS_CHECK(f->addFriend("10007", "10003", ""));
    QS_CHECK(g->requestJoin("10007", "1003", ""));   // 关注后直接加入
    QS_CHECK(weibo->post("10001", "测试一条微博"));
    QS_CHECK(weibo->timeline("10001").front().text == "测试一条微博");
}

QS_TEST(WeShopExtension) {
    PlatformQS platform(freshDir("weshop"));
    platform.start();
    auto* shop = dynamic_cast<WeShopServiceQS*>(platform.service("weshop"));
    QS_CHECK(shop != nullptr);
    QS_CHECK(shop->customerProducts("qiushanshop").size() == 3);
    QS_CHECK(shop->buy("qiushanshop", "g3", 1));
    QS_CHECK(!shop->buy("qiushanshop", "g3", 1));  // 库存不足
    QS_CHECK(shop->listProduct("qiushanshop", "书签", "", 300, 5));
    QS_CHECK(shop->ordersOf("zhouningshop").size() == 1);
}

QS_TEST(TwoProcessesShareDataThroughRevisions) {
    const std::string dir = freshDir("shared");
    PlatformQS a(dir);
    a.start();
    PlatformQS b(dir);
    b.start();
    QS_CHECK(a.service("qq")->friendCapability()->addFriend("10004", "10003", "你好"));
    QS_CHECK(b.service("qq")->friendCapability()->incomingRequests("10003").empty());  // b 尚未刷新
    QS_CHECK(b.refreshAll());
    QS_CHECK(b.service("qq")->friendCapability()->incomingRequests("10003").size() == 1);
    QS_CHECK(b.service("qq")->friendCapability()->addFriend("10004", "10007", ""));  // b 先读取最新数据再修改
    QS_CHECK(a.refreshAll());
    QS_CHECK(a.service("qq")->friendCapability()->outgoingRequests("10004").size() == 2);
}

QS_TEST(BindAccountVerifiesOtherService) {
    PlatformQS platform(freshDir("bind"));
    platform.start();
    ServiceBaseQS* wechat = platform.service("wechat");
    std::string id;
    QS_CHECK(wechat->registerAccount(FormQS{{"wxid", "jiangyu_wx"}, {"nickname", "江屿"}, {"password", "123456"},
                                            {"birthday", "2006-01-30"}, {"location", "哈尔滨"}}, id));
    QS_CHECK(!wechat->bindAccount(id, AccountRefQS("qq", "10004"), "bad"));
    QS_CHECK(!wechat->bindAccount(id, AccountRefQS("qq", "10001"), "123456"));  // 已被 qiushan 绑定
    QS_CHECK(wechat->bindAccount(id, AccountRefQS("qq", "10004"), "123456"));
    QS_CHECK(platform.linkedAccountIn(AccountRefQS("qq", "10004"), "wechat") == "jiangyu_wx");
}

int main() { return qs::TestHarnessQS::instance().run(); }

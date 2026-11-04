#include "QQServiceQS.h"

#include "DateQS.h"
#include "ServiceRegistryQS.h"

namespace qs {

QQServiceQS::QQServiceQS() : ServiceBaseQS("qq", "QQ"), friends_(*this, book_), groups_(*this, "qq") {}

std::vector<ServiceFeatureQS> QQServiceQS::features() const {
    return {{"chats", "消息", "chat"}, {"contacts", "联系人", "contacts"}, {"groups", "群聊", "group"}, {"me", "我", "me"}};
}

bool QQServiceQS::acceptsLinkedRegistrationFrom(const std::string& fromService) const { return fromService == "wechat"; }

ResultQS QQServiceQS::prepareAccountId(const FormQS&, std::string& id) {
    reloadIfChanged();
    id = accounts().nextNumericId(10001);  // QQ号：系统顺序分配
    return ResultQS::ok();
}

void QQServiceQS::finalizeAccountId(std::string& id) const { id = accounts().nextNumericId(10001); }

ResultQS QQServiceQS::createLinkedAccount(const UserProfileQS& profile, const std::string& password,
                                          const AccountRefQS& source, std::string& newId) {
    if (accounts().findLinkedTo(source) != nullptr) {
        return ResultQS::fail("该账号已经关联过QQ号");
    }
    newId = accounts().nextNumericId(10001);
    ServiceAccountQS account(newId, password, profile);
    account.profile().setRegisteredOn(DateQS::today());
    account.addLink(source);
    const ResultQS added = mutableAccounts().add(std::move(account));
    return added ? ResultQS::ok("已同时开通QQ，QQ号 " + newId) : added;
}

void QQServiceQS::onAccountRemoved(const std::string& id) {
    book_.forget(id);
    groups_.forget(id);
}

void QQServiceQS::loadData() {
    book_.fromJson(readDataFile("friends.json"));
    groups_.fromJson(readDataFile("groups.json", JsonQS::array()));
}

void QQServiceQS::saveData() const {
    writeDataFile("friends.json", book_.toJson());
    writeDataFile("groups.json", groups_.toJson());
}

void QQServiceQS::seedDemoData() {
    seedAccount("10001", "丘山", "2006-03-18", "吉林长春", "2016-07-01", {}, "认真把课设做完");
    seedAccount("10002", "林然", "2006-08-09", "吉林长春", "2017-02-14", {}, "今天也要好好跑步");
    seedAccount("10003", "许知", "2005-11-23", "辽宁沈阳", "2015-09-10", {}, "数学使人快乐");
    seedAccount("10004", "江屿", "2006-01-30", "黑龙江哈尔滨", "2018-06-01", {}, "CQ CQ");
    seedAccount("10005", "陈序", "2006-05-05", "吉林吉林", "2016-12-25", {}, "");
    seedAccount("10006", "周宁", "2005-10-01", "山东青岛", "2014-03-03", {}, "Linux 是一种生活方式");
    seedAccount("10007", "沈禾", "2006-12-12", "浙江杭州", "2019-01-20", {}, "");
    seedAccount("10008", "顾言", "2006-04-04", "江苏南京", "2020-08-08", {}, "少说话多做事");

    const std::pair<const char*, const char*> pairs[] = {{"10001", "10002"}, {"10001", "10003"}, {"10001", "10004"},
                                                        {"10001", "10005"}, {"10001", "10006"}, {"10002", "10003"},
                                                        {"10002", "10007"}, {"10005", "10007"}, {"10006", "10008"},
                                                        {"10003", "10006"}};
    for (const auto& p : pairs) book_.connect(p.first, p.second, "2025-09-01");
    if (FriendLinkQS* l = book_.link("10001", "10002")) { l->remark = "林然（同桌）"; l->tag = "同学"; }
    if (FriendLinkQS* l = book_.link("10001", "10006")) { l->tag = "同学"; }
    if (FriendLinkQS* l = book_.link("10001", "10004")) { l->tag = "朋友"; }
    book_.addRequest("10008", "10001", "我是顾言，C++课设群里的", "2026-10-18");

    GroupQS& cpp = groups_.seedGroup("1001", "C++课程设计交流群", "qq",
                                     {"10001", "10002", "10003", "10005", "10006", "10007"},
                                     "第九次课答辩，请提前彩排演示流程");
    cpp.setAdmin("10001", "10002", true);
    cpp.createSubgroup("10001", "答辩彩排小组", {"10001", "10002", "10003"});
    cpp.requestJoin("10008", "想一起讨论课设", true);
    groups_.seedGroup("1002", "校园跑团", "qq", {"10002", "10001", "10007", "10008"}, "周六早上七点操场集合");
    groups_.seedGroup("1003", "软件学院2024级", "qq",
                      {"10005", "10001", "10002", "10003", "10004", "10006", "10007", "10008"});
    groups_.seedGroup("1004", "数学竞赛备考", "qq", {"10003", "10001", "10006"});
    groups_.seedGroup("1005", "Linux实践互助", "qq", {"10006", "10002", "10004"});
    groups_.seedGroup("1006", "业余无线电爱好者", "qq", {"10004", "10008"});
}

QS_REGISTER_SERVICE(QQServiceQS)

}  // namespace qs

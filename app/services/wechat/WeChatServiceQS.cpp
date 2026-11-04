#include "WeChatServiceQS.h"

#include "DateQS.h"
#include "ServiceRegistryQS.h"

#include <cctype>
#include <random>

namespace qs {

WeChatServiceQS::WeChatServiceQS() : ServiceBaseQS("wechat", "微信"), friends_(*this, book_), groups_(*this, "wechat") {}

std::vector<ServiceFeatureQS> WeChatServiceQS::features() const {
    return {{"chats", "微信", "chat"}, {"contacts", "通讯录", "contacts"}, {"groups", "群聊", "group"}, {"me", "我", "me"}};
}

bool WeChatServiceQS::acceptsLinkedRegistrationFrom(const std::string& fromService) const { return fromService == "qq"; }

bool WeChatServiceQS::isValidWechatId(const std::string& id) {
    if (id.size() < 6 || id.size() > 20 || !std::isalpha(static_cast<unsigned char>(id[0]))) return false;
    for (unsigned char c : id) {
        if (!std::isalnum(c) && c != '_') return false;
    }
    return true;
}

std::string WeChatServiceQS::generateWechatId() {
    static const char alphabet[] = "abcdefghijkmnpqrstuvwxyz23456789";
    std::random_device device;
    std::mt19937 engine(device());
    std::uniform_int_distribution<int> pick(0, static_cast<int>(sizeof alphabet) - 2);
    std::string id = "wxid_";
    for (int i = 0; i < 8; ++i) id += alphabet[pick(engine)];
    return id;
}

std::vector<RegistrationFieldQS> WeChatServiceQS::idFields() const {
    return {{"wxid", "微信号", "字母开头6-20位，留空则系统分配", false, true}};
}

ResultQS WeChatServiceQS::prepareAccountId(const FormQS& form, std::string& id) {
    reloadIfChanged();
    const auto it = form.find("wxid");
    id = it == form.end() ? std::string() : it->second;
    if (id.empty()) {
        do { id = generateWechatId(); } while (accounts().contains(id));
        return ResultQS::ok();
    }
    if (!isValidWechatId(id)) return ResultQS::fail("微信号需以字母开头，6-20位字母、数字或下划线");
    if (accounts().contains(id)) return ResultQS::fail("微信号 " + id + " 已被占用");
    return ResultQS::ok();
}

ResultQS WeChatServiceQS::createLinkedAccount(const UserProfileQS& profile, const std::string& password,
                                              const AccountRefQS& source, std::string& newId) {
    if (accounts().findLinkedTo(source) != nullptr) return ResultQS::fail("该QQ号已绑定其他微信号");
    do { newId = generateWechatId(); } while (accounts().contains(newId));
    ServiceAccountQS account(newId, password, profile);
    account.profile().setRegisteredOn(DateQS::today());
    account.addLink(source);  // 微信与QQ的绑定关系由微信服务自己保存
    const ResultQS added = mutableAccounts().add(std::move(account));
    return added ? ResultQS::ok("已同时开通微信，微信号 " + newId) : added;
}

void WeChatServiceQS::onAccountRemoved(const std::string& id) {
    book_.forget(id);
    groups_.forget(id);
}

void WeChatServiceQS::loadData() {
    book_.fromJson(readDataFile("friends.json"));
    groups_.fromJson(readDataFile("groups.json", JsonQS::array()));
}

void WeChatServiceQS::saveData() const {
    writeDataFile("friends.json", book_.toJson());
    writeDataFile("groups.json", groups_.toJson());
}

void WeChatServiceQS::seedDemoData() {
    seedAccount("qiushan", "丘山", "2006-03-18", "吉林长春", "2018-09-01", {{"qq", "10001"}}, "在路上");
    seedAccount("linran", "林然", "2006-08-09", "吉林长春", "2018-10-02", {{"qq", "10002"}});
    seedAccount("xuzhi", "许知", "2005-11-23", "辽宁沈阳", "2017-03-15", {{"qq", "10003"}});
    seedAccount("chenxu", "陈序", "2006-05-05", "吉林吉林", "2019-01-01", {{"qq", "10005"}});
    seedAccount("zhouning", "周宁", "2005-10-01", "山东青岛", "2016-05-20", {{"qq", "10006"}});
    seedAccount("shenhe", "沈禾", "2006-12-12", "浙江杭州", "2020-02-02", {{"qq", "10007"}});
    seedAccount("guyan", "顾言", "2006-04-04", "江苏南京", "2021-07-07", {{"qq", "10008"}});

    const std::pair<const char*, const char*> pairs[] = {{"qiushan", "linran"}, {"qiushan", "chenxu"}, {"linran", "xuzhi"},
                                                        {"chenxu", "shenhe"}, {"zhouning", "guyan"}, {"xuzhi", "zhouning"},
                                                        {"linran", "shenhe"}};
    for (const auto& p : pairs) book_.connect(p.first, p.second, "2025-10-01");
    if (FriendLinkQS* l = book_.link("qiushan", "chenxu")) { l->tag = "室友"; }

    groups_.seedGroup("1001", "404宿舍", "wechat", {"qiushan", "linran", "chenxu"}, "周日大扫除");
    GroupQS& defense = groups_.seedGroup("1002", "课设答辩互助", "wechat", {"linran", "qiushan", "xuzhi", "zhouning"});
    defense.invite("linran", "shenhe");
    groups_.seedGroup("1003", "家人", "wechat", {"qiushan", "shenhe"});
    groups_.seedGroup("1004", "晨跑打卡", "wechat", {"chenxu", "qiushan", "shenhe"});
    groups_.seedGroup("1005", "二手书交易", "wechat", {"zhouning", "guyan", "xuzhi"});
    GroupQS& hometown = groups_.seedGroup("1006", "吉林老乡会", "wechat", {"shenhe", "chenxu", "guyan"});
    hometown.invite("chenxu", "qiushan");
}

QS_REGISTER_SERVICE(WeChatServiceQS)

}  // namespace qs

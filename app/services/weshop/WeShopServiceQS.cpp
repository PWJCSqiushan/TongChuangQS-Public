#include "WeShopServiceQS.h"

#include "DateQS.h"
#include "ServiceRegistryQS.h"

#include <algorithm>
#include <cctype>

namespace qs {

WeShopServiceQS::WeShopServiceQS()
    : ServiceBaseQS("weshop", "微商"), customers_(*this, book_, "客户"), groups_(*this, "wechat") {}

std::vector<ServiceFeatureQS> WeShopServiceQS::features() const {
    return {{"weshop.store", "小店", "store"}, {"contacts", "客户", "contacts"}, {"groups", "社群", "group"}, {"me", "我", "me"}};
}

std::vector<RegistrationFieldQS> WeShopServiceQS::idFields() const {
    return {{"shopId", "微商号", "字母开头，4-20位字母或数字", false, false}};
}

ResultQS WeShopServiceQS::prepareAccountId(const FormQS& form, std::string& id) {
    const auto it = form.find("shopId");
    id = it == form.end() ? std::string() : it->second;
    if (id.size() < 4 || id.size() > 20 || !std::isalpha(static_cast<unsigned char>(id[0])) ||
        !std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; })) {
        return ResultQS::fail("微商号需以字母开头，4-20位字母、数字或下划线");
    }
    reloadIfChanged();
    if (accounts().contains(id)) return ResultQS::fail("微商号 " + id + " 已被占用");
    return ResultQS::ok();
}

ResultQS WeShopServiceQS::createLinkedAccount(const UserProfileQS& profile, const std::string& password,
                                              const AccountRefQS& source, std::string& newId) {
    if (accounts().findLinkedTo(source) != nullptr) return ResultQS::fail("该微信号已开通过微商");
    newId = "shop_" + source.account();
    while (accounts().contains(newId)) newId += "1";
    ServiceAccountQS account(newId, password,
                             UserProfileQS(profile.nickname(), profile.birthday(), profile.location(), DateQS::today()));
    account.addLink(source);
    const ResultQS added = mutableAccounts().add(std::move(account));
    return added ? ResultQS::ok("已同时开通微商，微商号 " + newId) : added;
}

void WeShopServiceQS::onAccountRemoved(const std::string& id) {
    book_.forget(id);
    groups_.forget(id);
    products_.erase(std::remove_if(products_.begin(), products_.end(),
                                   [&](const WeShopProductQS& p) { return p.owner == id; }),
                    products_.end());
}

ResultQS WeShopServiceQS::listProduct(const std::string& me, const std::string& name, const std::string& description,
                                      int priceCents, int stock) {
    if (name.empty()) return ResultQS::fail("请填写商品名称");
    if (priceCents <= 0) return ResultQS::fail("价格必须大于0");
    if (stock <= 0) return ResultQS::fail("库存必须大于0");
    return transact([&]() -> ResultQS {
        products_.push_back(WeShopProductQS{"g" + std::to_string(nextId_++), me, name, description, priceCents, stock,
                                            DateQS::today()});
        return ResultQS::ok("商品「" + name + "」已上架");
    });
}

ResultQS WeShopServiceQS::removeProduct(const std::string& me, const std::string& productId) {
    return transact([&]() -> ResultQS {
        const auto it = std::find_if(products_.begin(), products_.end(),
                                     [&](const WeShopProductQS& p) { return p.id == productId; });
        if (it == products_.end()) return ResultQS::fail("商品不存在");
        if (it->owner != me) return ResultQS::fail("只能下架自己的商品");
        products_.erase(it);
        return ResultQS::ok("已下架");
    });
}

ResultQS WeShopServiceQS::buy(const std::string& me, const std::string& productId, int quantity) {
    if (quantity <= 0) return ResultQS::fail("购买数量至少为1");
    return transact([&]() -> ResultQS {
        for (auto& p : products_) {
            if (p.id != productId) continue;
            if (p.owner == me) return ResultQS::fail("不能购买自己的商品");
            if (!customers_.areFriends(me, p.owner)) return ResultQS::fail("只能在客户（好友）的小店下单");
            if (p.stock < quantity) return ResultQS::fail("库存不足，仅剩 " + std::to_string(p.stock) + " 件");
            p.stock -= quantity;
            orders_.push_back(WeShopOrderQS{"o" + std::to_string(nextId_++), me, p.owner, p.name, quantity,
                                            p.priceCents * quantity, DateQS::today()});
            return ResultQS::ok("下单成功：" + p.name + " ×" + std::to_string(quantity));
        }
        return ResultQS::fail("商品不存在");
    });
}

std::vector<WeShopProductQS> WeShopServiceQS::productsOf(const std::string& owner) const {
    std::vector<WeShopProductQS> result;
    for (const auto& p : products_) {
        if (p.owner == owner) result.push_back(p);
    }
    return result;
}

std::vector<WeShopProductQS> WeShopServiceQS::customerProducts(const std::string& me) const {
    std::vector<WeShopProductQS> result;
    for (const auto& p : products_) {
        if (p.owner != me && customers_.areFriends(me, p.owner)) result.push_back(p);
    }
    return result;
}

std::vector<WeShopOrderQS> WeShopServiceQS::ordersOf(const std::string& me) const {
    std::vector<WeShopOrderQS> result;
    for (const auto& o : orders_) {
        if (o.buyer == me || o.seller == me) result.push_back(o);
    }
    return result;
}

void WeShopServiceQS::loadData() {
    book_.fromJson(readDataFile("customers.json"));
    groups_.fromJson(readDataFile("groups.json", JsonQS::array()));
    const JsonQS shop = readDataFile("shop.json");
    products_.clear();
    orders_.clear();
    for (const auto& p : shop.value("products", JsonQS::array())) products_.push_back(WeShopProductQS::fromJson(p));
    for (const auto& o : shop.value("orders", JsonQS::array())) orders_.push_back(WeShopOrderQS::fromJson(o));
    nextId_ = shop.value("next", 1LL);
}

void WeShopServiceQS::saveData() const {
    writeDataFile("customers.json", book_.toJson());
    writeDataFile("groups.json", groups_.toJson());
    JsonQS products = JsonQS::array();
    for (const auto& p : products_) products.push_back(p.toJson());
    JsonQS orders = JsonQS::array();
    for (const auto& o : orders_) orders.push_back(o.toJson());
    writeDataFile("shop.json", JsonQS{{"products", products}, {"orders", orders}, {"next", nextId_}});
}

void WeShopServiceQS::seedDemoData() {
    seedAccount("qiushanshop", "丘山的小店", "2006-03-18", "吉林长春", "2026-09-01", {{"wechat", "qiushan"}}, "自制手账与贴纸");
    seedAccount("linranshop", "林然跑步装备", "2006-08-09", "吉林长春", "2026-09-05", {{"wechat", "linran"}});
    seedAccount("zhouningshop", "周宁二手书", "2005-10-01", "山东青岛", "2026-09-10", {{"wechat", "zhouning"}});
    book_.connect("qiushanshop", "linranshop", "2026-09-06");
    book_.connect("qiushanshop", "zhouningshop", "2026-09-11");
    products_.push_back(WeShopProductQS{"g1", "qiushanshop", "手绘课程表贴纸", "防水哑光，一套12张", 990, 30, "2026-09-02"});
    products_.push_back(WeShopProductQS{"g2", "linranshop", "反光跑步臂包", "夜跑必备，可放手机", 3900, 8, "2026-09-06"});
    products_.push_back(WeShopProductQS{"g3", "zhouningshop", "《C++ Primer》第5版（九成新）", "有少量笔记", 4500, 1, "2026-09-11"});
    products_.push_back(WeShopProductQS{"g4", "zhouningshop", "《数学分析》上下册", "考研用过，品相好", 3000, 2, "2026-09-12"});
    nextId_ = 5;
    groups_.seedGroup("1001", "小店老客户群", "wechat", {"qiushanshop", "linranshop"});
    groups_.seedGroup("1002", "校园二手交流", "wechat", {"zhouningshop", "qiushanshop"});
    groups_.seedGroup("1003", "跑步装备团购", "wechat", {"linranshop"});
    groups_.seedGroup("1004", "手账爱好者", "wechat", {"qiushanshop"});
    groups_.seedGroup("1005", "开学季闲置", "wechat", {"zhouningshop"});
    groups_.seedGroup("1006", "微商新手交流", "wechat", {"linranshop", "zhouningshop"});
}

QS_REGISTER_SERVICE(WeShopServiceQS)

}  // namespace qs

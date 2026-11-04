#pragma once

#include "JsonQS.h"

#include <string>


namespace qs {

/** 微商小店里的一件商品。价格以"分"为单位保存，避免浮点误差。 */
class WeShopProductQS {
public:
    std::string id;
    std::string owner;
    std::string name;
    std::string description;
    int priceCents = 0;
    int stock = 0;
    std::string createdOn;

    std::string priceText() const {
        return "¥" + std::to_string(priceCents / 100) + "." + (priceCents % 100 < 10 ? "0" : "") +
               std::to_string(priceCents % 100);
    }
    JsonQS toJson() const {
        return JsonQS{{"id", id},       {"owner", owner}, {"name", name},          {"description", description},
                      {"price", priceCents}, {"stock", stock}, {"createdOn", createdOn}};
    }
    static WeShopProductQS fromJson(const JsonQS& j) {
        return WeShopProductQS{j.value("id", std::string()),   j.value("owner", std::string()),
                               j.value("name", std::string()), j.value("description", std::string()),
                               j.value("price", 0),            j.value("stock", 0),
                               j.value("createdOn", std::string())};
    }
};

/** 一笔订单。 */


}  // namespace qs

#include "WeShopOrderQS.h"

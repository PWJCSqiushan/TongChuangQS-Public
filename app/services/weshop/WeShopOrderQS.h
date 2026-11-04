#pragma once

#include "JsonQS.h"
#include <string>

namespace qs {

class WeShopOrderQS {
public:
    std::string id;
    std::string buyer;
    std::string seller;
    std::string productName;
    int quantity = 0;
    int totalCents = 0;
    std::string date;

    JsonQS toJson() const {
        return JsonQS{{"id", id}, {"buyer", buyer}, {"seller", seller}, {"product", productName},
                      {"quantity", quantity}, {"total", totalCents}, {"date", date}};
    }
    static WeShopOrderQS fromJson(const JsonQS& j) {
        return WeShopOrderQS{j.value("id", std::string()),      j.value("buyer", std::string()),
                             j.value("seller", std::string()),  j.value("product", std::string()),
                             j.value("quantity", 0),            j.value("total", 0),
                             j.value("date", std::string())};
    }
};

}  // namespace qs

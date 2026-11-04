#pragma once

#include "JsonQS.h"
#include <string>
#include <vector>

namespace qs {

class GroupApplicationQS {
public:
    std::string account;
    std::string note;
    std::string date;
    JsonQS toJson() const { return JsonQS{{"account", account}, {"note", note}, {"date", date}}; }
    static GroupApplicationQS fromJson(const JsonQS& j) {
        return GroupApplicationQS{j.value("account", std::string()), j.value("note", std::string()),
                                  j.value("date", std::string())};
    }
};

}  // namespace qs

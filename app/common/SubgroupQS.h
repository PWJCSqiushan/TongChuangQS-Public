#pragma once

#include "JsonQS.h"
#include <string>
#include <vector>

namespace qs {

class SubgroupQS {
public:
    std::string id;
    std::string name;
    std::string creator;
    std::string createdOn;
    std::vector<std::string> members;
    JsonQS toJson() const {
        return JsonQS{{"id", id}, {"name", name}, {"creator", creator}, {"createdOn", createdOn}, {"members", members}};
    }
    static SubgroupQS fromJson(const JsonQS& j) {
        SubgroupQS s;
        s.id = j.value("id", std::string());
        s.name = j.value("name", std::string());
        s.creator = j.value("creator", std::string());
        s.createdOn = j.value("createdOn", std::string());
        s.members = j.value("members", std::vector<std::string>());
        return s;
    }
};

}  // namespace qs

#pragma once

#include "JsonQS.h"

#include <cstdint>
#include <set>
#include <string>

namespace qs {

/** 一条微博。 */
class WeiboPostQS {
public:
    std::string id;
    std::string author;
    std::string text;
    std::int64_t time = 0;
    std::set<std::string> likes;

    JsonQS toJson() const {
        return JsonQS{{"id", id}, {"author", author}, {"text", text}, {"time", time}, {"likes", likes}};
    }
    static WeiboPostQS fromJson(const JsonQS& j) {
        WeiboPostQS p;
        p.id = j.value("id", std::string());
        p.author = j.value("author", std::string());
        p.text = j.value("text", std::string());
        p.time = j.value("time", static_cast<std::int64_t>(0));
        p.likes = j.value("likes", std::set<std::string>());
        return p;
    }
};

}  // namespace qs

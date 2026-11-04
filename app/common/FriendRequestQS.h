#pragma once

#include "JsonQS.h"

#include <string>


namespace qs {

/** 一条待处理的好友申请。 */
class FriendRequestQS {
public:
    std::string id;
    std::string from;
    std::string to;
    std::string note;
    std::string date;

    JsonQS toJson() const { return JsonQS{{"id", id}, {"from", from}, {"to", to}, {"note", note}, {"date", date}}; }
    static FriendRequestQS fromJson(const JsonQS& j) {
        return FriendRequestQS{j.value("id", std::string()), j.value("from", std::string()), j.value("to", std::string()),
                               j.value("note", std::string()), j.value("date", std::string())};
    }
};

/** 界面展示用的好友视图（已合并昵称、备注、分组）。 */


}  // namespace qs

#include "FriendViewQS.h"

#pragma once

#include "JsonQS.h"
#include <string>

namespace qs {

class FriendViewQS {
public:
    std::string account;
    std::string nickname;
    std::string remark;
    std::string tag;
    std::string since;
    /** 关系说明，如"互相关注""仅你关注"。 */
    std::string relation;

    const std::string& displayName() const { return remark.empty() ? nickname : remark; }
};

}  // namespace qs

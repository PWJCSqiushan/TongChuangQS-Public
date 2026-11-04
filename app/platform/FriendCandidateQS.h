#pragma once

#include "LoginSessionQS.h"
#include "ServiceBaseQS.h"
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace qs {

class FriendCandidateQS {
public:
    std::string account;      // 在目标服务中的账号
    std::string nickname;
    std::string sourceAccount;  // 在来源服务中的账号
    std::string reason;
};

}  // namespace qs

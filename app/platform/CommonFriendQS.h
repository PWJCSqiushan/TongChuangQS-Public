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

class CommonFriendQS {
public:
    std::string nickname;
    AccountRefQS first;
    AccountRefQS second;
};

}  // namespace qs

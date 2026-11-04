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

class BusLogEntryQS {
public:
    std::int64_t time = 0;
    std::string from;
    std::string to;
    std::string kind;
    bool ok = false;
    std::string message;
};

}  // namespace qs

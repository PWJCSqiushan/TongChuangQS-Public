#pragma once

#include "AccountRefQS.h"
#include <cstdint>
#include <filesystem>
#include <string>

namespace qs {

class PresenceQS {
public:
    AccountRefQS ref;
    std::string nickname;
    std::string host;
    unsigned short port = 0;
    long long pid = 0;
    std::int64_t heartbeat = 0;
};

}  // namespace qs

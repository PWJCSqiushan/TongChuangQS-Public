#pragma once

#include "ChatNodeQS.h"
#include "PlatformQS.h"
#include <memory>
#include <string>
#include <vector>

namespace qs {

class ConversationViewQS {
public:
    std::string key;
    std::string title;
    std::string preview;
    std::int64_t time = 0;
    int unread = 0;
    bool isGroup = false;
    bool online = false;
    std::string peer;  // 私聊对方账号或群号
};

}  // namespace qs

#pragma once

#include "ChatMessageQS.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace qs {

class ConversationQS {
public:
    std::string key;    // "p:账号" / "g:群号"
    std::string title;  // 对方昵称或群名
    int unread = 0;
    std::vector<ChatMessageQS> messages;

    std::int64_t lastTime() const { return messages.empty() ? 0 : messages.back().time; }
    std::string preview() const;
};

}  // namespace qs

#pragma once

#include "ChatMessageQS.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace qs {

class ChatSearchHitQS {
public:
    std::string conversationKey;
    std::string conversationTitle;
    ChatMessageQS message;
};

}  // namespace qs

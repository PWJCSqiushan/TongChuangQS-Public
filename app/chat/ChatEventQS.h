#pragma once

#include "AccountRefQS.h"
#include "ChatHistoryQS.h"
#include "FileLockQS.h"
#include "NetworkRuntimeQS.h"
#include "PeerLinkQS.h"
#include "PresenceBoardQS.h"
#include "ResultQS.h"
#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace qs {

class ChatEventQS {
public:
    enum class KindQS { MessageReceived, StatusChanged, PresenceChanged };
    KindQS kind = KindQS::MessageReceived;
    AccountRefQS owner;
    std::string conversation;
    std::string messageId;
    std::string text;
};

}  // namespace qs

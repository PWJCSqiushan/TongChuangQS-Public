#pragma once

#include "ConsoleFeatureQS.h"

namespace qs {

/** 控制台"消息"菜单：私聊、群聊、聊天记录查询。 */
class ConsoleChatsQS final : public ConsoleFeatureQS {
public:
    void run(ConsoleContextQS& context) override;
};

}  // namespace qs

#pragma once

#include "ConsoleFeatureQS.h"

namespace qs {

/** 控制台"联系人"菜单：只依赖 FriendCapabilityQS 接口，QQ/微信/微博/微商通用。 */
class ConsoleContactsQS final : public ConsoleFeatureQS {
public:
    void run(ConsoleContextQS& context) override;
};

}  // namespace qs

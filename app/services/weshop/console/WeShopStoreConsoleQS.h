#pragma once

#include "ConsoleFeatureQS.h"

namespace qs {

/** 微商特色功能"小店"的控制台菜单（属于微商服务自己的代码，自注册接入）。 */
class WeShopStoreConsoleQS final : public ConsoleFeatureQS {
public:
    void run(ConsoleContextQS& context) override;
};

}  // namespace qs

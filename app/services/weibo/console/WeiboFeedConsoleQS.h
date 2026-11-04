#pragma once

#include "ConsoleFeatureQS.h"

namespace qs {

/** 微博特色功能"首页"的控制台菜单（属于微博服务自己的代码，自注册接入）。 */
class WeiboFeedConsoleQS final : public ConsoleFeatureQS {
public:
    void run(ConsoleContextQS& context) override;
};

}  // namespace qs

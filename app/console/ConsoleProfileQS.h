#pragma once

#include "ConsoleFeatureQS.h"

namespace qs {

/** 控制台"我"菜单：资料、密码、账号绑定、注销。 */
class ConsoleProfileQS final : public ConsoleFeatureQS {
public:
    void run(ConsoleContextQS& context) override;
};

}  // namespace qs

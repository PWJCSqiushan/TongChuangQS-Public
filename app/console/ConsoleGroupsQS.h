#pragma once

#include "ConsoleFeatureQS.h"

namespace qs {

/** 控制台"群"菜单：只依赖 GroupCapabilityQS 接口，规则差异由各群的管理模式决定。 */
class ConsoleGroupsQS final : public ConsoleFeatureQS {
public:
    void run(ConsoleContextQS& context) override;
};

}  // namespace qs

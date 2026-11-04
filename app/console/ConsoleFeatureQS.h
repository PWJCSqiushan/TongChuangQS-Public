#pragma once

#include "ClientQS.h"
#include "ConsoleIOQS.h"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "ConsoleContextQS.h"

namespace qs {

/** 控制台功能菜单运行时可用的上下文。 */


/**
 * 控制台中的一个功能菜单（抽象基类）。
 * 通用能力（消息/联系人/群/我）与服务特色功能（微博首页、微商小店）都实现本接口，
 * 并按功能类型注册到 ConsoleFeatureRegistryQS；主菜单根据服务声明的功能列表动态生成。
 */
class ConsoleFeatureQS {
public:
    virtual ~ConsoleFeatureQS() = default;
    virtual void run(ConsoleContextQS& context) = 0;
};


}  // namespace qs

#define QS_REGISTER_CONSOLE_FEATURE(Type, kind) \
    static const ::qs::ConsoleFeatureAutoRegisterQS<Type> qsConsoleFeature##Type(kind);

#include "ConsoleFeatureRegistryQS.h"
#include "ConsoleFeatureAutoRegisterQS.h"

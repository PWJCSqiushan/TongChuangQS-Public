#pragma once

#include "ServiceBaseQS.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>


namespace qs {

/**
 * 服务注册表（工厂 + 自注册）。
 * 每个服务在自己的 .cpp 末尾写一行 QS_REGISTER_SERVICE(类名)，
 * 程序启动时该静态对象自动把"创建这个服务的方法"登记到注册表；
 * 平台启动时向注册表要"全部服务"，因此新增服务不需要修改平台的任何一行代码。
 */
class ServiceRegistryQS {
public:
    using FactoryQS = std::function<std::unique_ptr<ServiceBaseQS>()>;

    static ServiceRegistryQS& instance();
    void add(FactoryQS factory);
    /** 创建所有已注册服务，按 displayOrder 排序。 */
    std::vector<std::unique_ptr<ServiceBaseQS>> createAll() const;

private:
    std::vector<FactoryQS> factories_;
};


}  // namespace qs

#define QS_REGISTER_SERVICE(Type) static const ::qs::ServiceAutoRegisterQS<Type> qsServiceAutoRegister##Type;

#include "ServiceAutoRegisterQS.h"

#pragma once

#include "GroupPolicyQS.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>


namespace qs {

/**
 * 群管理模式注册表（工厂）。每种模式在自己的 .cpp 里用 QS_REGISTER_GROUP_POLICY 自注册，
 * 存档时只保存 kind 字符串，读档时由注册表创建对应子类对象。
 * 新增一种群管理模式只需新增一个子类文件，无需修改 GroupQS 或任何服务代码。
 */
class GroupPolicyRegistryQS {
public:
    using FactoryQS = std::function<std::unique_ptr<GroupPolicyQS>()>;

    static GroupPolicyRegistryQS& instance();
    void add(const std::string& kind, FactoryQS factory);
    std::unique_ptr<GroupPolicyQS> create(const std::string& kind) const;
    std::vector<std::string> kinds() const;

private:
    std::map<std::string, FactoryQS> factories_;
};


}  // namespace qs

#define QS_REGISTER_GROUP_POLICY(Type) \
    static const ::qs::GroupPolicyAutoRegisterQS<Type> qsGroupPolicyAutoRegister##Type;

#include "GroupPolicyAutoRegisterQS.h"

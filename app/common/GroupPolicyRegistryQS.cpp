#include "GroupPolicyRegistryQS.h"

namespace qs {

GroupPolicyRegistryQS& GroupPolicyRegistryQS::instance() {
    static GroupPolicyRegistryQS registry;
    return registry;
}

void GroupPolicyRegistryQS::add(const std::string& kind, FactoryQS factory) { factories_[kind] = std::move(factory); }

std::unique_ptr<GroupPolicyQS> GroupPolicyRegistryQS::create(const std::string& kind) const {
    const auto it = factories_.find(kind);
    return it == factories_.end() ? nullptr : it->second();
}

std::vector<std::string> GroupPolicyRegistryQS::kinds() const {
    std::vector<std::string> result;
    for (const auto& entry : factories_) {
        result.push_back(entry.first);
    }
    return result;
}

}  // namespace qs

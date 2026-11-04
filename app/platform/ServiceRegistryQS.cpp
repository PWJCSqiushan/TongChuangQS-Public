#include "ServiceRegistryQS.h"

#include <algorithm>

namespace qs {

ServiceRegistryQS& ServiceRegistryQS::instance() {
    static ServiceRegistryQS registry;
    return registry;
}

void ServiceRegistryQS::add(FactoryQS factory) { factories_.push_back(std::move(factory)); }

std::vector<std::unique_ptr<ServiceBaseQS>> ServiceRegistryQS::createAll() const {
    std::vector<std::unique_ptr<ServiceBaseQS>> services;
    for (const auto& factory : factories_) {
        services.push_back(factory());
    }
    std::sort(services.begin(), services.end(),
              [](const auto& a, const auto& b) { return a->displayOrder() < b->displayOrder(); });
    return services;
}

}  // namespace qs

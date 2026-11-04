#pragma once

#include "ClientQS.h"
#include "ConsoleIOQS.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace qs {

class ConsoleFeatureQS;

class ConsoleFeatureRegistryQS {
public:
    using FactoryQS = std::function<std::unique_ptr<ConsoleFeatureQS>()>;
    static ConsoleFeatureRegistryQS& instance() {
        static ConsoleFeatureRegistryQS registry;
        return registry;
    }
    void add(const std::string& kind, FactoryQS factory) { factories_[kind] = std::move(factory); }
    std::unique_ptr<ConsoleFeatureQS> create(const std::string& kind) const;

private:
    std::map<std::string, FactoryQS> factories_;
};

}  // namespace qs

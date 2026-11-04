#pragma once

#include "ClientQS.h"
#include "ConsoleIOQS.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "ConsoleFeatureRegistryQS.h"

namespace qs {

template <class FeatureQS>
class ConsoleFeatureAutoRegisterQS {
public:
    explicit ConsoleFeatureAutoRegisterQS(const char* kind) {
        ConsoleFeatureRegistryQS::instance().add(kind, [] { return std::make_unique<FeatureQS>(); });
    }
};

}  // namespace qs

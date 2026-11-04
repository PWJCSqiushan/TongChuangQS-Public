#pragma once

#include "ServiceBaseQS.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "ServiceRegistryQS.h"

namespace qs {

template <class ServiceQS>
class ServiceAutoRegisterQS {
public:
    ServiceAutoRegisterQS() {
        ServiceRegistryQS::instance().add([] { return std::make_unique<ServiceQS>(); });
    }
};

}  // namespace qs

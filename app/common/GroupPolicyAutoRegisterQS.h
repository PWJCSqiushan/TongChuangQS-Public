#pragma once

#include "GroupPolicyQS.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "GroupPolicyRegistryQS.h"

namespace qs {

template <class PolicyQS>
class GroupPolicyAutoRegisterQS {
public:
    GroupPolicyAutoRegisterQS() {
        PolicyQS probe;
        GroupPolicyRegistryQS::instance().add(probe.kind(), [] { return std::make_unique<PolicyQS>(); });
    }
};

}  // namespace qs

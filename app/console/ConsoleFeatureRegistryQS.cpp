#include "ConsoleFeatureQS.h"

namespace qs {
std::unique_ptr<ConsoleFeatureQS> ConsoleFeatureRegistryQS::create(const std::string& kind) const {
    const auto it = factories_.find(kind);
    return it == factories_.end() ? nullptr : it->second();
}
}  // namespace qs

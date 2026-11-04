#pragma once

#include <map>
#include <string>

namespace qs {

class RegistrationFieldQS {
public:
    std::string key;
    std::string label;
    std::string placeholder;
    bool secret = false;
    bool optional = false;
};

}  // namespace qs

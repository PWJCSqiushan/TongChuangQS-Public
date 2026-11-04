#pragma once

#include "JsonQS.h"
#include <string>

namespace qs {

class ServiceRequestQS {
public:
    std::string from;   // 发起服务 id
    std::string to;     // 目标服务 id
    std::string kind;   // 请求类型，如 "verify_credentials"、"register_linked"
    JsonQS payload = JsonQS::object();
};

}  // namespace qs

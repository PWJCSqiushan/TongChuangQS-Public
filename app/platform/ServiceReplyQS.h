#pragma once

#include "JsonQS.h"
#include <string>

namespace qs {

class ServiceReplyQS {
public:
    bool ok = false;
    std::string message;
    JsonQS payload = JsonQS::object();

    static ServiceReplyQS success(JsonQS payload = JsonQS::object(), std::string message = "") {
        ServiceReplyQS r;
        r.ok = true;
        r.payload = std::move(payload);
        r.message = std::move(message);
        return r;
    }
    static ServiceReplyQS failure(std::string message) {
        ServiceReplyQS r;
        r.message = std::move(message);
        return r;
    }
};

}  // namespace qs

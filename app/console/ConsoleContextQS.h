#pragma once

#include "ClientQS.h"
#include "ConsoleIOQS.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace qs {

class ConsoleContextQS {
public:
    ClientQS& client;
    ConsoleIOQS& io;
    std::recursive_mutex& lock;  // 与后台收信线程互斥
    std::string service;
    /** 当前正在查看的会话（后台线程据此实时打印新消息）。 */
    std::string activeConversation;

    ServiceBaseQS& svc() const { return *client.platform().service(service); }
    std::string me() const { return client.accountOf(service); }
    void show(const ResultQS& r) const { r ? io.ok(r.message()) : io.warn(r.message()); }
};

}  // namespace qs

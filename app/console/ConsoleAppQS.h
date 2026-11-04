#pragma once

#include "ClientQS.h"
#include "ConsoleFeatureQS.h"
#include "ConsoleIOQS.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace qs {

/**
 * 控制台版主程序。流程与图形界面一致（老师要求的"先选服务"）：
 *   选择服务 → 该服务的登录/注册（或一键进入）→ 该服务自己的功能菜单。
 * 功能菜单由服务声明的 features() 动态生成，每项交给注册的 ConsoleFeatureQS 处理。
 * 后台线程负责收发网络消息并实时提示新消息。
 */
class ConsoleAppQS {
public:
    explicit ConsoleAppQS(const std::string& dataRootUtf8);
    ~ConsoleAppQS();
    int run();

private:
    void serviceEntry(const std::string& service);
    bool loginFlow(const std::string& service);
    bool registerFlow(const std::string& service);
    /** 返回 true 表示退出程序。 */
    bool serviceHome(const std::string& service);
    void showBusLog();
    void pumpLoop();
    std::string badge(const std::string& service) const;

    ClientQS client_;
    ConsoleIOQS io_;
    std::recursive_mutex lock_;
    ConsoleContextQS context_;
    std::atomic<bool> pumping_{false};
    std::thread pump_;
};

}  // namespace qs

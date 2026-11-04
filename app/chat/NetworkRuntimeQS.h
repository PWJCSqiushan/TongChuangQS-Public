#pragma once

namespace qs {

/**
 * 网络库生命周期（RAII）。Windows 下使用 Winsock 前必须调用 WSAStartup，
 * 程序结束时调用 WSACleanup；本类在构造/析构中自动完成，其他平台为空操作。
 * 需要链接 ws2_32.lib（见 CMakeLists.txt）。
 */
class NetworkRuntimeQS {
public:
    NetworkRuntimeQS();
    ~NetworkRuntimeQS();
    NetworkRuntimeQS(const NetworkRuntimeQS&) = delete;
    NetworkRuntimeQS& operator=(const NetworkRuntimeQS&) = delete;
    bool ok() const noexcept { return ok_; }

private:
    bool ok_ = false;
};

}  // namespace qs

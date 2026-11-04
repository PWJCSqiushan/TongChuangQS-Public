#pragma once

#include <cstdint>
#include <string>

namespace qs {

/**
 * TCP 套接字的 RAII 封装：析构时自动关闭，禁止拷贝、允许移动。
 * Windows 下基于 Winsock（bind / listen / accept / connect / send / recv），
 * 其他平台基于 BSD socket，两者接口一致。头文件不包含 winsock2.h，避免污染其他模块。
 */
class SocketQS {
public:
    SocketQS() = default;
    ~SocketQS();
    SocketQS(const SocketQS&) = delete;
    SocketQS& operator=(const SocketQS&) = delete;
    SocketQS(SocketQS&& other) noexcept;
    SocketQS& operator=(SocketQS&& other) noexcept;

    /** 在 host:port 上监听；port 为 0 时由系统分配空闲端口。 */
    static SocketQS listenOn(const std::string& host, unsigned short port, std::string& error);
    /** 连接 host:port，最多等待 timeoutMs 毫秒。 */
    static SocketQS connectTo(const std::string& host, unsigned short port, int timeoutMs, std::string& error);

    bool valid() const noexcept;
    unsigned short localPort() const;
    std::string peerAddress() const;

    /** 等待新连接，超时返回无效套接字。 */
    SocketQS acceptFor(int timeoutMs) const;
    /** 发送全部字节；失败返回 false。 */
    bool sendAll(const std::uint8_t* data, std::size_t size) const;
    /** 接收：>0 为字节数，0 为对方关闭，-1 为超时，-2 为错误。 */
    int receiveFor(std::uint8_t* buffer, std::size_t capacity, int timeoutMs) const;
    /** 只关闭读写方向而不释放句柄：用于从其他线程唤醒正在等待接收的读线程。 */
    void shutdownBoth() const noexcept;
    /** 关闭并释放套接字。 */
    void close() noexcept;

private:
    explicit SocketQS(std::uintptr_t handle) noexcept : handle_(handle) {}
    std::uintptr_t handle_ = kInvalid;
    static constexpr std::uintptr_t kInvalid = ~static_cast<std::uintptr_t>(0);
};

}  // namespace qs

#pragma once

#include "SocketQS.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace qs {

/**
 * 与另一个窗口之间的一条 TCP 连接。
 * 每条连接有一个独立的读线程：不停接收字节、按"4字节长度+内容"拆出完整消息帧，
 * 交给回调放入节点的收件队列；发送在调用者线程进行，用互斥锁保证帧不交错。
 */
class PeerLinkQS {
public:
    using FrameSinkQS = std::function<void(int linkId, std::string payload)>;

    PeerLinkQS(int id, SocketQS socket, FrameSinkQS sink);
    ~PeerLinkQS();
    PeerLinkQS(const PeerLinkQS&) = delete;
    PeerLinkQS& operator=(const PeerLinkQS&) = delete;

    void start();
    void stop();
    bool send(const std::string& payload);
    bool alive() const noexcept { return alive_; }
    int id() const noexcept { return id_; }
    std::string remoteAddress() const { return remoteAddress_; }

    /** 对端节点标识（握手后由主线程写入）。 */
    std::string endpoint;

private:
    void run();

    int id_;
    SocketQS socket_;
    FrameSinkQS sink_;
    std::string remoteAddress_;
    std::thread reader_;
    std::atomic<bool> alive_{true};
    std::atomic<bool> stopping_{false};
    std::mutex sendMutex_;
};

}  // namespace qs

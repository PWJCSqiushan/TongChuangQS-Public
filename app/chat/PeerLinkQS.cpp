#include "PeerLinkQS.h"

#include "FrameCodecQS.h"

namespace qs {

PeerLinkQS::PeerLinkQS(int id, SocketQS socket, FrameSinkQS sink)
    : id_(id), socket_(std::move(socket)), sink_(std::move(sink)) {
    remoteAddress_ = socket_.peerAddress();
}

PeerLinkQS::~PeerLinkQS() { stop(); }

void PeerLinkQS::start() { reader_ = std::thread(&PeerLinkQS::run, this); }

void PeerLinkQS::stop() {
    stopping_ = true;
    socket_.shutdownBoth();  // 唤醒读线程
    if (reader_.joinable()) reader_.join();
    socket_.close();
    alive_ = false;
}

bool PeerLinkQS::send(const std::string& payload) {
    if (!alive_) return false;
    try {
        const std::vector<std::uint8_t> frame = FrameCodecQS::encode(payload);
        std::lock_guard<std::mutex> lock(sendMutex_);
        if (!socket_.sendAll(frame.data(), frame.size())) {
            alive_ = false;
            return false;
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void PeerLinkQS::run() {
    std::vector<std::uint8_t> buffer;
    std::uint8_t chunk[4096];
    while (!stopping_) {
        const int n = socket_.receiveFor(chunk, sizeof chunk, 300);
        if (n == -1) continue;  // 超时：继续等待
        if (n <= 0) break;      // 对方关闭或出错
        buffer.insert(buffer.end(), chunk, chunk + n);
        try {
            std::string payload;
            // 一次接收可能包含半条或多条消息：循环拆出所有完整帧。
            while (FrameCodecQS::tryDecode(buffer, payload)) {
                sink_(id_, std::move(payload));
            }
        } catch (const std::exception&) {
            break;  // 非法数据：断开连接
        }
    }
    alive_ = false;
}

}  // namespace qs

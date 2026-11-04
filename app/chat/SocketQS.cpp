#include "SocketQS.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using NativeSocketQS = SOCKET;
#define QS_CLOSE_SOCKET closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
using NativeSocketQS = int;
#define QS_CLOSE_SOCKET ::close
#endif

#include <utility>

namespace qs {

namespace {

NativeSocketQS native(std::uintptr_t h) { return static_cast<NativeSocketQS>(h); }

bool waitReadable(NativeSocketQS s, int timeoutMs) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(s, &set);
    timeval tv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    return select(static_cast<int>(s) + 1, &set, nullptr, nullptr, &tv) > 0;
}

void setBlocking(NativeSocketQS s, bool blocking) {
#ifdef _WIN32
    u_long mode = blocking ? 0 : 1;
    ioctlsocket(s, FIONBIO, &mode);
#else
    const int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK));
#endif
}

bool toAddress(const std::string& host, unsigned short port, sockaddr_in& address) {
    address = sockaddr_in{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    return inet_pton(AF_INET, host.c_str(), &address.sin_addr) == 1;
}

}  // namespace

SocketQS::~SocketQS() { close(); }

SocketQS::SocketQS(SocketQS&& other) noexcept : handle_(std::exchange(other.handle_, kInvalid)) {}

SocketQS& SocketQS::operator=(SocketQS&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = std::exchange(other.handle_, kInvalid);
    }
    return *this;
}

bool SocketQS::valid() const noexcept { return handle_ != kInvalid; }

SocketQS SocketQS::listenOn(const std::string& host, unsigned short port, std::string& error) {
    sockaddr_in address;
    if (!toAddress(host, port, address)) {
        error = "无效的监听地址 " + host;
        return SocketQS();
    }
    NativeSocketQS s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SocketQS result(static_cast<std::uintptr_t>(s));
#ifdef _WIN32
    if (s == INVALID_SOCKET) {
#else
    if (s < 0) {
#endif
        error = "无法创建套接字";
        return SocketQS();
    }
#ifndef _WIN32
    int reuse = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
#endif
    if (::bind(s, reinterpret_cast<sockaddr*>(&address), sizeof address) != 0) {
        error = "端口 " + std::to_string(port) + " 已被占用";
        return SocketQS();
    }
    if (::listen(s, 16) != 0) {
        error = "监听失败";
        return SocketQS();
    }
    return result;
}

SocketQS SocketQS::connectTo(const std::string& host, unsigned short port, int timeoutMs, std::string& error) {
    sockaddr_in address;
    if (!toAddress(host, port, address)) {
        error = "无效的地址 " + host;
        return SocketQS();
    }
    NativeSocketQS s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SocketQS result(static_cast<std::uintptr_t>(s));
    setBlocking(s, false);
    ::connect(s, reinterpret_cast<sockaddr*>(&address), sizeof address);
    fd_set writable;
    FD_ZERO(&writable);
    FD_SET(s, &writable);
    fd_set failed;
    FD_ZERO(&failed);
    FD_SET(s, &failed);
    timeval tv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    if (select(static_cast<int>(s) + 1, nullptr, &writable, &failed, &tv) <= 0 || FD_ISSET(s, &failed)) {
        error = "连接 " + host + ":" + std::to_string(port) + " 超时或被拒绝";
        return SocketQS();
    }
    int code = 0;
#ifdef _WIN32
    int length = sizeof code;
    getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&code), &length);
#else
    socklen_t length = sizeof code;
    getsockopt(s, SOL_SOCKET, SO_ERROR, &code, &length);
#endif
    if (code != 0) {
        error = "连接被拒绝";
        return SocketQS();
    }
    setBlocking(s, true);
    int noDelay = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof noDelay);
    return result;
}

unsigned short SocketQS::localPort() const {
    sockaddr_in address{};
#ifdef _WIN32
    int length = sizeof address;
#else
    socklen_t length = sizeof address;
#endif
    if (getsockname(native(handle_), reinterpret_cast<sockaddr*>(&address), &length) != 0) return 0;
    return ntohs(address.sin_port);
}

std::string SocketQS::peerAddress() const {
    sockaddr_in address{};
#ifdef _WIN32
    int length = sizeof address;
#else
    socklen_t length = sizeof address;
#endif
    if (getpeername(native(handle_), reinterpret_cast<sockaddr*>(&address), &length) != 0) return "";
    char text[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &address.sin_addr, text, sizeof text);
    return text;
}

SocketQS SocketQS::acceptFor(int timeoutMs) const {
    if (!valid() || !waitReadable(native(handle_), timeoutMs)) return SocketQS();
    NativeSocketQS client = ::accept(native(handle_), nullptr, nullptr);
#ifdef _WIN32
    if (client == INVALID_SOCKET) return SocketQS();
#else
    if (client < 0) return SocketQS();
#endif
    int noDelay = 1;
    setsockopt(client, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof noDelay);
    return SocketQS(static_cast<std::uintptr_t>(client));
}

bool SocketQS::sendAll(const std::uint8_t* data, std::size_t size) const {
    std::size_t sent = 0;
    while (sent < size) {
#ifdef _WIN32
        const int n = ::send(native(handle_), reinterpret_cast<const char*>(data + sent), static_cast<int>(size - sent), 0);
#else
        const ssize_t n = ::send(native(handle_), data + sent, size - sent, MSG_NOSIGNAL);
#endif
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

int SocketQS::receiveFor(std::uint8_t* buffer, std::size_t capacity, int timeoutMs) const {
    if (!valid()) return -2;
    if (!waitReadable(native(handle_), timeoutMs)) return valid() ? -1 : -2;
#ifdef _WIN32
    const int n = ::recv(native(handle_), reinterpret_cast<char*>(buffer), static_cast<int>(capacity), 0);
#else
    const ssize_t n = ::recv(native(handle_), buffer, capacity, 0);
#endif
    if (n < 0) return -2;
    return static_cast<int>(n);
}

void SocketQS::shutdownBoth() const noexcept {
    if (handle_ == kInvalid) return;
#ifdef _WIN32
    ::shutdown(native(handle_), SD_BOTH);
#else
    ::shutdown(native(handle_), SHUT_RDWR);
#endif
}

void SocketQS::close() noexcept {
    if (handle_ == kInvalid) return;
    const NativeSocketQS s = native(handle_);
    handle_ = kInvalid;
#ifdef _WIN32
    ::shutdown(s, SD_BOTH);
#else
    ::shutdown(s, SHUT_RDWR);
#endif
    QS_CLOSE_SOCKET(s);
}

}  // namespace qs

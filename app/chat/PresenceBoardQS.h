#pragma once

#include "AccountRefQS.h"

#include <cstdint>
#include <filesystem>
#include <string>

#include "PresenceQS.h"

namespace qs {

/** 在线信息：某账号由哪个窗口（进程）托管，通过哪个地址和端口可以连上。 */


/**
 * 在线公告板：每个登录了聊天服务的窗口定期把"我在线、我的端口"写到
 * data/_presence/ 下，其他窗口读取它即可自动找到好友——无需手动输入 IP 和端口。
 * 心跳超过 8 秒未更新视为离线。
 */
class PresenceBoardQS {
public:
    explicit PresenceBoardQS(const std::filesystem::path& directory);

    void publish(const PresenceQS& presence) const;
    void withdraw(const AccountRefQS& ref, long long pid, unsigned short port) const;
    bool lookup(const AccountRefQS& ref, PresenceQS& out) const;
    bool isOnline(const AccountRefQS& ref) const;

    static constexpr std::int64_t kTimeoutMs = 8000;

private:
    std::filesystem::path fileFor(const AccountRefQS& ref) const;
    std::filesystem::path directory_;
};

}  // namespace qs

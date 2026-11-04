#pragma once

#include <filesystem>

namespace qs {

/**
 * 跨进程文件锁（RAII）。两个演示窗口共用同一份数据目录，
 * 修改某个服务的数据前先加锁 → 重新读取 → 修改 → 写回 → 解锁，
 * 避免两个进程同时写文件互相覆盖。
 * Windows 用独占打开锁文件实现，其他平台用 flock。
 */
class FileLockQS {
public:
    explicit FileLockQS(const std::filesystem::path& lockFile, int timeoutMs = 5000);
    ~FileLockQS();

    FileLockQS(const FileLockQS&) = delete;
    FileLockQS& operator=(const FileLockQS&) = delete;

    bool locked() const noexcept { return locked_; }

private:
    bool locked_ = false;
#ifdef _WIN32
    void* handle_ = nullptr;
#else
    int fd_ = -1;
#endif
};

}  // namespace qs

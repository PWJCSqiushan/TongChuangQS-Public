#include "FileLockQS.h"

#include <chrono>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace qs {

FileLockQS::FileLockQS(const std::filesystem::path& lockFile, int timeoutMs) {
    std::error_code ignored;
    std::filesystem::create_directories(lockFile.parent_path(), ignored);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (true) {
#ifdef _WIN32
        HANDLE h = CreateFileW(lockFile.wstring().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            handle_ = h;
            locked_ = true;
            return;
        }
#else
        if (fd_ < 0) {
            fd_ = ::open(lockFile.c_str(), O_RDWR | O_CREAT, 0644);
        }
        if (fd_ >= 0 && ::flock(fd_, LOCK_EX | LOCK_NB) == 0) {
            locked_ = true;
            return;
        }
#endif
        if (std::chrono::steady_clock::now() >= deadline) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

FileLockQS::~FileLockQS() {
#ifdef _WIN32
    if (handle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(handle_));
    }
#else
    if (fd_ >= 0) {
        if (locked_) {
            ::flock(fd_, LOCK_UN);
        }
        ::close(fd_);
    }
#endif
}

}  // namespace qs

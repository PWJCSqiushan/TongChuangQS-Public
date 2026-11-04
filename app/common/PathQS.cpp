#include "PathQS.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <climits>
#endif

namespace qs {

std::filesystem::path PathQS::fromUtf8(const std::string& utf8) {
#ifdef _WIN32
    if (utf8.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), length);
    return std::filesystem::path(wide);
#else
    return std::filesystem::path(utf8);
#endif
}

std::string PathQS::toUtf8(const std::filesystem::path& path) {
#ifdef _WIN32
    const std::wstring wide = path.wstring();
    if (wide.empty()) {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), length, nullptr, nullptr);
    return utf8;
#else
    return path.string();
#endif
}

std::filesystem::path PathQS::join(const std::string& rootUtf8, const std::string& childUtf8) {
    return fromUtf8(rootUtf8) / fromUtf8(childUtf8);
}

std::filesystem::path PathQS::executableDirectory() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH * 4];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(sizeof buffer / sizeof buffer[0]));
    return std::filesystem::path(std::wstring(buffer, length)).parent_path();
#else
    char buffer[PATH_MAX];
    const ssize_t length = readlink("/proc/self/exe", buffer, sizeof buffer - 1);
    if (length <= 0) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(std::string(buffer, static_cast<std::size_t>(length))).parent_path();
#endif
}

}  // namespace qs

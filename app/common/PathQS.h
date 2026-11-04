#pragma once

#include <filesystem>
#include <string>

namespace qs {

/**
 * UTF-8 字符串与文件路径之间的转换。项目目录含中文（如"丘山"），
 * Windows 下必须经宽字符路径访问文件，否则会出现乱码或找不到文件。
 */
class PathQS {
public:
    static std::filesystem::path fromUtf8(const std::string& utf8);
    static std::string toUtf8(const std::filesystem::path& path);
    static std::filesystem::path join(const std::string& rootUtf8, const std::string& childUtf8);
    /** 当前可执行文件所在目录。 */
    static std::filesystem::path executableDirectory();
};

}  // namespace qs

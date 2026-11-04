#pragma once

#include <string>
#include <vector>

namespace qs {

/**
 * 控制台输入输出。Windows 控制台默认不是 UTF-8，直接用 std::getline 读中文会乱码，
 * 因此 Windows 下改用 ReadConsoleW 读宽字符再转为 UTF-8。
 */
class ConsoleIOQS {
public:
    ConsoleIOQS();
    std::string readLine(const std::string& prompt);
    /** 读取 [min,max] 内的整数；输入非法时重新提示。 */
    int readChoice(const std::string& prompt, int min, int max);
    bool confirm(const std::string& question);
    void print(const std::string& text);
    void title(const std::string& text);
    void ok(const std::string& text);
    void warn(const std::string& text);
    bool eof() const noexcept { return eof_; }

private:
    bool eof_ = false;
};

}  // namespace qs

#include "ConsoleIOQS.h"

#include <cstdio>
#include <iostream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace qs {

ConsoleIOQS::ConsoleIOQS() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

std::string ConsoleIOQS::readLine(const std::string& prompt) {
    std::fputs(prompt.c_str(), stdout);
    std::fflush(stdout);
#ifdef _WIN32
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(input, &mode)) {
        wchar_t buffer[2048];
        DWORD read = 0;
        if (!ReadConsoleW(input, buffer, 2047, &read, nullptr) || read == 0) {
            eof_ = true;
            return "";
        }
        std::wstring wide(buffer, read);
        while (!wide.empty() && (wide.back() == L'\n' || wide.back() == L'\r')) wide.pop_back();
        const int length = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        std::string utf8(static_cast<std::size_t>(length), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), utf8.data(), length, nullptr, nullptr);
        return utf8;
    }
#endif
    std::string line;
    if (!std::getline(std::cin, line)) {
        eof_ = true;
        return "";
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

int ConsoleIOQS::readChoice(const std::string& prompt, int min, int max) {
    while (!eof_) {
        const std::string text = readLine(prompt);
        try {
            std::size_t used = 0;
            const int value = std::stoi(text, &used);
            if (used == text.size() && value >= min && value <= max) return value;
        } catch (...) {
        }
        if (!eof_) warn("请输入 " + std::to_string(min) + " 到 " + std::to_string(max) + " 之间的数字");
    }
    return 0;
}

bool ConsoleIOQS::confirm(const std::string& question) {
    const std::string answer = readLine(question + "（y/n）：");
    return answer == "y" || answer == "Y" || answer == "是";
}

void ConsoleIOQS::print(const std::string& text) {
    std::fputs((text + "\n").c_str(), stdout);
    std::fflush(stdout);
}

void ConsoleIOQS::title(const std::string& text) { print("\n━━━━━━━━ " + text + " ━━━━━━━━"); }
void ConsoleIOQS::ok(const std::string& text) { print("  ✔ " + text); }
void ConsoleIOQS::warn(const std::string& text) { print("  ✘ " + text); }

}  // namespace qs

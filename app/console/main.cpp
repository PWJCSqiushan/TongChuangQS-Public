// 控制台版入口：SocialQS.exe [--data 数据目录]
#include "ConsoleAppQS.h"
#include "PathQS.h"

#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

namespace {

std::vector<std::string> utf8Arguments(int argc, char** argv) {
    std::vector<std::string> args;
#ifdef _WIN32
    (void)argc;
    (void)argv;
    int count = 0;
    wchar_t** wide = CommandLineToArgvW(GetCommandLineW(), &count);
    for (int i = 0; i < count; ++i) args.push_back(qs::PathQS::toUtf8(std::filesystem::path(wide[i])));
    LocalFree(wide);
#else
    for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);
#endif
    return args;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string> args = utf8Arguments(argc, argv);
    std::string dataRoot = qs::PathQS::toUtf8(qs::PathQS::executableDirectory() / "data");
    for (std::size_t i = 1; i + 1 < args.size(); ++i) {
        if (args[i] == "--data") dataRoot = args[i + 1];
    }
    qs::ConsoleAppQS app(dataRoot);
    return app.run();
}

#pragma once

// 极简测试框架：不依赖第三方库，方便在 VS 与 g++ 下直接运行。
#include <cstdio>
#include <functional>
#include <string>
#include <vector>


namespace qs {

class TestHarnessQS {
public:
    static TestHarnessQS& instance() {
        static TestHarnessQS harness;
        return harness;
    }
    void add(const char* name, std::function<void()> body) { cases_.push_back({name, std::move(body)}); }
    void fail(const char* file, int line, const std::string& what) {
        ++failures_;
        std::printf("  FAIL %s:%d  %s\n", file, line, what.c_str());
    }
    void pass() { ++checks_; }
    int run() {
        for (auto& c : cases_) {
            const int before = failures_;
            c.second();
            std::printf("[%s] %s\n", failures_ == before ? " OK " : "FAIL", c.first);
        }
        std::printf("\n%d checks passed, %d failed, %zu cases\n", checks_, failures_, cases_.size());
        return failures_ == 0 ? 0 : 1;
    }

private:
    std::vector<std::pair<const char*, std::function<void()>>> cases_;
    int failures_ = 0;
    int checks_ = 0;
};


}  // namespace qs

#define QS_TEST(name)                                                    \
    static void name();                                                  \
    static const ::qs::TestCaseQS qsTestCase##name(#name, &name);        \
    static void name()

#define QS_CHECK(cond)                                                                       \
    do {                                                                                     \
        if (cond) ::qs::TestHarnessQS::instance().pass();                                    \
        else ::qs::TestHarnessQS::instance().fail(__FILE__, __LINE__, #cond);                \
    } while (0)

#define QS_CHECK_MSG(cond, msg)                                                              \
    do {                                                                                     \
        if (cond) ::qs::TestHarnessQS::instance().pass();                                    \
        else ::qs::TestHarnessQS::instance().fail(__FILE__, __LINE__, std::string(#cond) + " | " + (msg)); \
    } while (0)

#include "TestCaseQS.h"

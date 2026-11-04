#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include "TestHarnessQS.h"

namespace qs {

class TestCaseQS {
public:
    TestCaseQS(const char* name, std::function<void()> body) { TestHarnessQS::instance().add(name, std::move(body)); }
};

}  // namespace qs

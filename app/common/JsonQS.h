#pragma once

// 统一的 JSON 类型别名（nlohmann/json 3.12，单头文件，MIT 许可）。
#include <nlohmann/json.hpp>

namespace qs {
using JsonQS = nlohmann::json;
}

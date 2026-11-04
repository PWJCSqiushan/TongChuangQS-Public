#pragma once

#include "JsonQS.h"

#include <filesystem>

namespace qs {

/**
 * JSON 文件读写。写入时先写临时文件再整体替换，
 * 替换失败时保留已提交的旧文件；不使用先删目标文件的覆盖方式。
 */
class JsonFileQS {
public:
    /** 文件不存在或解析失败时返回 fallback。 */
    static JsonQS read(const std::filesystem::path& file, const JsonQS& fallback = JsonQS::object());
    /** 成功返回 true。 */
    static bool write(const std::filesystem::path& file, const JsonQS& value);
};

}  // namespace qs

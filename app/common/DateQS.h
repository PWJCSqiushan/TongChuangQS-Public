#pragma once

#include <cstdint>
#include <string>

namespace qs {

/**
 * 日期与时间工具。日期统一用 "YYYY-MM-DD" 文本保存，便于在 JSON 文件中阅读；
 * 时间戳统一用毫秒（UTC epoch）。
 */
class DateQS {
public:
    static std::string today();
    static bool isValid(const std::string& ymd);
    /** 从 from 到 to 经过的完整年数（用于计算"T龄"与年龄）。 */
    static int fullYearsBetween(const std::string& from, const std::string& to);
    static std::int64_t nowMillis();
    /** 本地时间 "HH:MM"。 */
    static std::string clockText(std::int64_t millis);
    /** 本地日期 "YYYY-MM-DD"。 */
    static std::string dateText(std::int64_t millis);
    /** 聊天列表用的简短时间：今天显示 HH:MM，昨天显示"昨天"，更早显示 MM-DD。 */
    static std::string shortText(std::int64_t millis);
};

}  // namespace qs

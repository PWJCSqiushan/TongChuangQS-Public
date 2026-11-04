#include "DateQS.h"

#include <chrono>
#include <cstdio>
#include <ctime>

namespace qs {

namespace {

std::tm localTm(std::int64_t millis) {
    const std::time_t seconds = static_cast<std::time_t>(millis / 1000);
    std::tm value{};
#ifdef _WIN32
    localtime_s(&value, &seconds);
#else
    localtime_r(&seconds, &value);
#endif
    return value;
}

bool parse(const std::string& ymd, int& y, int& m, int& d) {
    if (ymd.size() != 10 || ymd[4] != '-' || ymd[7] != '-') {
        return false;
    }
    return std::sscanf(ymd.c_str(), "%4d-%2d-%2d", &y, &m, &d) == 3;
}

}  // namespace

std::string DateQS::today() { return dateText(nowMillis()); }

bool DateQS::isValid(const std::string& ymd) {
    int y = 0, m = 0, d = 0;
    if (!parse(ymd, y, m, d) || y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1) {
        return false;
    }
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    const int limit = (m == 2 && leap) ? 29 : days[m - 1];
    return d <= limit;
}

int DateQS::fullYearsBetween(const std::string& from, const std::string& to) {
    int y1 = 0, m1 = 0, d1 = 0, y2 = 0, m2 = 0, d2 = 0;
    if (!parse(from, y1, m1, d1) || !parse(to, y2, m2, d2)) {
        return 0;
    }
    int years = y2 - y1;
    if (m2 < m1 || (m2 == m1 && d2 < d1)) {
        --years;
    }
    return years < 0 ? 0 : years;
}

std::int64_t DateQS::nowMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string DateQS::clockText(std::int64_t millis) {
    const std::tm t = localTm(millis);
    char buffer[8];
    std::snprintf(buffer, sizeof buffer, "%02d:%02d", t.tm_hour, t.tm_min);
    return buffer;
}

std::string DateQS::dateText(std::int64_t millis) {
    const std::tm t = localTm(millis);
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    return buffer;
}

std::string DateQS::shortText(std::int64_t millis) {
    if (millis <= 0) {
        return "";
    }
    const std::int64_t now = nowMillis();
    const std::string day = dateText(millis);
    if (day == dateText(now)) {
        return clockText(millis);
    }
    if (day == dateText(now - 24LL * 3600 * 1000)) {
        return "昨天";
    }
    return day.substr(5);
}

}  // namespace qs

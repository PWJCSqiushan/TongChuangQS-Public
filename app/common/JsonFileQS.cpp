#include "JsonFileQS.h"

#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace qs {

JsonQS JsonFileQS::read(const std::filesystem::path& file, const JsonQS& fallback) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        return fallback;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const JsonQS parsed = JsonQS::parse(buffer.str(), nullptr, false);
    return parsed.is_discarded() ? fallback : parsed;
}

bool JsonFileQS::write(const std::filesystem::path& file, const JsonQS& value) {
    std::error_code error;
    std::filesystem::create_directories(file.parent_path(), error);
    std::filesystem::path temp = file;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            return false;
        }
        out << value.dump(2);
        out.flush();
        out.close();
        if (out.fail()) {
            return false;
        }
    }
#ifdef _WIN32
    // Never remove the committed file to make room. A failed replacement must
    // leave it intact (including when another program holds the destination).
    return MoveFileExW(temp.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::filesystem::rename(temp, file, error);
    return !error;
#endif
}

}  // namespace qs

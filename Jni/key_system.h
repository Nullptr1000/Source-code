#pragma once
#include <string>
#include <cstdint>

namespace rc::license {
enum class Duration { Day, Week, Month, Year };

struct Result {
    bool valid;
    std::string message;
    std::int64_t expires_at;
};

std::string generate_key(Duration duration);
Result validate_key(const std::string& key, std::int64_t now = 0);
bool save_key(const std::string& key, const char* path = "/sdcard/RC/license.key");
std::string load_key(const char* path = "/sdcard/RC/license.key");
} // namespace rc::license

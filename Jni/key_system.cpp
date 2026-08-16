#include "key_system.h"
#include <ctime>
#include <fstream>
#include <random>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace {
static std::int64_t duration_seconds(rc::license::Duration d) {
    switch (d) {
        case rc::license::Duration::Day:   return 1LL * 24 * 60 * 60;
        case rc::license::Duration::Week:  return 7LL * 24 * 60 * 60;
        case rc::license::Duration::Month: return 30LL * 24 * 60 * 60;
        case rc::license::Duration::Year:  return 365LL * 24 * 60 * 60;
    }
    return 0;
}

static std::string random_part(std::size_t n) {
    static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> dist(0, sizeof(alphabet) - 2);
    std::string s;
    s.reserve(n);
    for (std::size_t i = 0; i < n; ++i) s += alphabet[dist(gen)];
    return s;
}

static bool parse_expiry(const std::string& key, std::int64_t& expiry) {
    // Format: VAL-<unix-expiry>-<random>
    if (key.rfind("VAL-", 0) != 0) return false;
    const auto p1 = key.find('-', 4);
    if (p1 == std::string::npos || key.find('-', p1 + 1) != std::string::npos) return false;
    const std::string ts = key.substr(4, p1 - 4);
    const std::string rnd = key.substr(p1 + 1);
    if (ts.empty() || rnd.size() != 12) return false;
    for (char c : ts) if (!std::isdigit((unsigned char)c)) return false;
    for (char c : rnd) if (!(std::isdigit((unsigned char)c) || (c >= 'A' && c <= 'Z'))) return false;
    try { expiry = std::stoll(ts); } catch (...) { return false; }
    return expiry > 0;
}
}

namespace rc::license {

std::string generate_key(Duration duration) {
    const std::int64_t expiry = static_cast<std::int64_t>(std::time(nullptr)) + duration_seconds(duration);
    return "VAL-" + std::to_string(expiry) + "-" + random_part(12);
}

Result validate_key(const std::string& key, std::int64_t now) {
    if (now == 0) now = static_cast<std::int64_t>(std::time(nullptr));
    std::int64_t expiry = 0;
    if (!parse_expiry(key, expiry))
        return {false, "Invalid key format.", 0};
    if (expiry <= now)
        return {false, "This key has expired.", expiry};
    return {true, "Key accepted.", expiry};
}

bool save_key(const std::string& key, const char* path) {
    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;
    f << key;
    return true;
}

std::string load_key(const char* path) {
    std::ifstream f(path);
    if (!f) return {};
    std::string key;
    std::getline(f, key);
    return key;
}

} // namespace rc::license

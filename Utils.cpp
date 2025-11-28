// Utils.cpp (or a similar name)
#include "Utils.hpp"
#include <ctime>
#include <chrono>

std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}
static long elapsedMs(const std::chrono::steady_clock::time_point &start) {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count();
}


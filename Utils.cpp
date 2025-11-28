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


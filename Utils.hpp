// Utils.hpp (or a similar name)
#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include<chrono>
// Declaration of the function
std::string getCurrentTimestamp();
inline long elapsedMs(const std::chrono::steady_clock::time_point &start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start
           ).count();
}
#endif // UTILS_HPP


// Utils.hpp (or a similar name)
#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include<chrono>
// Declaration of the function
std::string getCurrentTimestamp();
static long elapsedMs(const std::chrono::steady_clock::time_point &start);
#endif // UTILS_HPP


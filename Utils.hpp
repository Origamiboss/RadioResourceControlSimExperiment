// Utils.hpp (or a similar name)
#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

// Declaration of the function
std::string getCurrentTimestamp();
static inline long elapsedMs(const std::chrono::high_resolution_clock::time_point &start);
#endif // UTILS_HPP


#pragma once

#include <cstdint>
#include <string>

/** Format a statistic number the way the curses sidebar does (grouped digits). */
std::string FormatStatNumber(std::int64_t number);

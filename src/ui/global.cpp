#include "core/domain.hpp"

#include <optional>
#include <string>

Statistic statistic;
Statistic disk_statistic;
Mode mode;
int user_umask;
bool print_time;
bool resize_request;
bool bypass_small_window;
std::optional<std::string> initial_directory;

#pragma once

#include <filesystem>
#include <optional>

std::optional<std::filesystem::path> GetXdgCachePath();
std::optional<std::filesystem::path> GetXdgConfigPath();

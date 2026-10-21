#pragma once

#include <filesystem>
#include <optional>

bool IsRoot(const std::string& path);
bool IsReadable(const std::filesystem::path& path);
bool IsWriteable(const std::filesystem::path& path);
std::optional<std::filesystem::path> GetXdgCachePath();
std::optional<std::filesystem::path> GetXdgConfigPath();

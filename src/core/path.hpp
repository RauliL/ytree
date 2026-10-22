#pragma once

#include <filesystem>
#include <optional>

inline bool IsAbsolute(std::string_view path)
{
  return (
    !path.empty() &&
    path[0] == std::filesystem::path::preferred_separator
  );
}

inline bool IsRoot(std::string_view path)
{
  return (
    path.size() == 1 &&
    path[0] == std::filesystem::path::preferred_separator
  );
}

bool IsReadable(const std::filesystem::path& path);
bool IsWriteable(const std::filesystem::path& path);
std::optional<std::string_view> GetExtension(std::string_view path);
std::optional<std::filesystem::path> GetXdgCachePath();
std::optional<std::filesystem::path> GetXdgConfigPath();

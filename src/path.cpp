#include "./ytree.hpp"

#include <peelo/xdg.hpp>

static inline bool IsAbsolute(const std::string& path)
{
  return !path.empty() &&
    path[0] == std::filesystem::path::preferred_separator;
}

std::filesystem::path FileEntry::GetPath() const
{
  return ::GetPath(Dir().get()) / name;
}

std::filesystem::path FileEntry::GetRealPath() const
{
  if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
  {
    return GetPath();
  }

  if (meta.IsLink())
  {
    if (IsAbsolute(symlink_target))
    {
      return symlink_target;
    }

    return ::GetPath(Dir().get()) / symlink_target;
  }

  return ::GetPath(Dir().get()) / name;
}

std::optional<std::filesystem::path> GetXdgConfigPath()
{
  if (const auto config_dir = peelo::xdg::config_dir())
  {
    return *config_dir / "ytree";
  }

  return std::nullopt;
}

std::optional<std::filesystem::path> GetXdgCachePath()
{
  if (const auto cache_dir = peelo::xdg::cache_dir())
  {
    return *cache_dir / "ytree";
  }

  return std::nullopt;
}

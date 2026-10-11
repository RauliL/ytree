#include "ytree.h"

#include <peelo/xdg.hpp>

bool DoStat(const std::filesystem::path& path, struct stat& st)
{
#if defined(S_IFLNK)
  return !lstat(path.c_str(), &st);
#else
  return !stat(path.c_str(), &st);
#endif
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

#include "ytree.h"

#include <peelo/xdg.hpp>
#include <system_error>

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

  if (S_ISLNK(stat_struct.st_mode))
  {
    if (IsAbsolute(symlink_target))
    {
      return symlink_target;
    }

    return ::GetPath(Dir().get()) / symlink_target;
  }

  return ::GetPath(Dir().get()) / name;
}

bool DoStat(const std::filesystem::path& path, struct stat& st)
{
#if defined(S_IFLNK)
  return !lstat(path.c_str(), &st);
#else
  return !stat(path.c_str(), &st);
#endif
}

void DoStat(
  const std::filesystem::path& path,
  struct stat& st,
  std::error_code& ec
)
{
  bool result;

  ec.clear();

#if defined(S_IFLNK)
  result = !lstat(path.c_str(), &st);
#else
  result = !stat(path.c_str(), &st);
#endif

  if (!result)
  {
    ec.assign(errno, std::system_category());
  }
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

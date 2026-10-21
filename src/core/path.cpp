#include "core/api.hpp"

#include <peelo/xdg.hpp>

static inline bool IsAbsolute(const std::string& path)
{
  return !path.empty() &&
    path[0] == std::filesystem::path::preferred_separator;
}

bool IsRoot(const std::string& path)
{
  return path.length() == 1 &&
    path[0] == std::filesystem::path::preferred_separator;
}

static bool CheckPermissions(
  const std::filesystem::path& path,
  const std::filesystem::perms& owner,
  const std::filesystem::perms& group,
  const std::filesystem::perms& others
)
{
  std::error_code ec;
  const auto status = std::filesystem::status(path, ec);

  if (ec)
  {
    return false;
  }

  const auto perms = status.permissions();

  return (
    ((perms & owner) != std::filesystem::perms::none) |
    ((perms & group) != std::filesystem::perms::none) |
    ((perms & others) != std::filesystem::perms::none)
  );
}

bool IsReadable(const std::filesystem::path& path)
{
  return CheckPermissions(
    path,
    std::filesystem::perms::owner_read,
    std::filesystem::perms::group_read,
    std::filesystem::perms::others_read
  );
}

bool IsWriteable(const std::filesystem::path& path)
{
  return CheckPermissions(
    path,
    std::filesystem::perms::owner_write,
    std::filesystem::perms::group_write,
    std::filesystem::perms::others_write
  );
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

std::filesystem::path DirEntry::GetPath() const
{
  std::string result;

  // Parents are owned by the tree, so the raw cursor stays valid while the
  // temporary `shared_ptr` returned by `Parent()` goes away.
  for (auto ptr = this; ptr;)
  {
    const auto parent = ptr->Parent();
    const auto is_root = IsRoot(ptr->name);

    if (!is_root)
    {
      result.insert(0, ptr->name);
    }
    // A nameless root (archive placeholder) doesn't contribute a separator.
    if (parent && !parent->name.empty())
    {
      result.insert(result.begin(), std::filesystem::path::preferred_separator);
    }

    ptr = parent.get();
  }

  return result;
}

std::filesystem::path GetPath(const DirEntry* entry)
{
  return entry ? entry->GetPath() : std::filesystem::path{};
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

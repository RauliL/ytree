#include "core/api.hpp"
#include "core/dir-entry.hpp"
#include "core/path.hpp"

std::filesystem::path FileEntry::GetPath() const
{
  const auto dir = Dir();

  if (dir)
  {
    return dir->GetPath() / name;
  }

  return name;
}

std::filesystem::path FileEntry::GetRealPath() const
{
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    const auto dir = Dir();
    std::string_view filename;

    if (meta.IsLink())
    {
      if (IsAbsolute(symlink_target))
      {
        return symlink_target;
      }
      filename = symlink_target;
    } else {
      filename = name;
    }
    if (dir)
    {
      return dir->GetPath() / filename;
    }

    return filename;
  }

  return GetPath();
}

std::optional<std::string_view> FileEntry::GetExtension() const
{
  return ::GetExtension(name);
}

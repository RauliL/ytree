#include "core/dir-entry.hpp"
#include "core/path.hpp"

#include <peelo/xdg.hpp>

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
    ((perms & owner) != std::filesystem::perms::none) ||
    ((perms & group) != std::filesystem::perms::none) ||
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

std::optional<std::string_view> GetExtension(std::string_view path)
{
  std::string_view::size_type dot;

  if (path.empty() || path == "." || path == "..")
  {
    return std::nullopt;
  }

  dot = path.rfind('.');

  if (dot == 0 || dot == std::string_view::npos)
  {
    return std::nullopt;
  }

  return path.substr(dot);
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

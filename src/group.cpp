#include "ytree.h"

#if !defined(_WIN32)

std::optional<std::string> GetGroupName(gid_t gid)
{
  const auto group = getgrgid(gid);

  return group ? std::make_optional<std::string>(group->gr_name) : std::nullopt;
}

std::optional<int> GetGroupId(const std::string& name)
{
  const auto group = getgrnam(name.c_str());

  return group ? std::make_optional(group->gr_gid) : std::nullopt;
}

#else /* _WIN32 */

std::optional<std::string> GetGroupName(gid_t /*gid*/)
{
  return std::nullopt;
}

std::optional<int> GetGroupId(const std::string& /*name*/)
{
  return std::nullopt;
}

#endif

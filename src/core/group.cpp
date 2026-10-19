#include "core/api.hpp"

std::optional<std::string> GetGroupName(int gid)
{
#if !defined(_WIN32)
  if (const auto group = getgrgid(static_cast<gid_t>(gid)))
  {
    return group->gr_name;
  }
#endif

  return std::nullopt;
}

std::optional<int> GetGroupId(const std::string& name)
{
#if !defined(_WIN32)
  if (const auto group = getgrnam(name.c_str()))
  {
    return group->gr_gid;
  }
#endif

  return std::nullopt;
}

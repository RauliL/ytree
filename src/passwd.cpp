#include "ytree.h"

#if defined(_WIN32)
# include <pwd.h>
#endif

std::optional<std::string> GetPasswdName(uid_t uid)
{
  auto pwd = getpwuid(uid);

  return pwd ? std::make_optional<std::string>(pwd->pw_name) : std::nullopt;
}

std::optional<int> GetPasswdUid(const std::string& name)
{
  auto pwd = getpwnam(name.c_str());

  return pwd ? std::make_optional(pwd->pw_uid) : std::nullopt;
}

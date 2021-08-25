#include "ytree.h"

#if defined(_WIN32)
# include <pwd.h>
#endif

const char* GetPasswdName(uid_t uid)
{
  auto pwd = getpwuid(uid);

  return pwd ? pwd->pw_name : nullptr;
}

const char* GetDisplayPasswdName(uid_t uid)
{
  auto pwd = getpwuid(uid);

  return pwd ? pwd->pw_name : nullptr;
}

int GetPasswdUid(const char* name)
{
  auto pwd = getpwnam(name);

  return pwd ? pwd->pw_uid : -1;
}

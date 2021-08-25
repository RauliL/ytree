#include "ytree.h"

#if defined(_WIN32)
# include <grp.h>
#endif

const char* GetGroupName(gid_t gid)
{
  auto group = getgrgid(gid);

  return group ? group->gr_name : nullptr;
}

const char* GetDisplayGroupName(gid_t gid)
{
  auto group = getgrgid(gid);

  return group ? group->gr_name : nullptr;
}

int GetGroupId(const char* name)
{
  auto group = getgrnam(name);

  return group ? group->gr_gid : -1;
}

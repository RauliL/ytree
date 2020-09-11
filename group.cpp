/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/group.c,v 1.16 2020/03/15 14:55:13 werner Exp $
 *
 * Handhabung von Gruppen-Nummern / Namen
 *
 ***************************************************************************/
#include "ytree.h"

#include <vector>

#ifdef WIN32

#include "grp.h"

#else

struct Group
{
  int gid;
  std::string name;
  std::string display_name;
};

static std::vector<Group> group_array;

extern struct group *getgrent(void);
#if !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined( __FreeBSD__ ) && !defined( OSF1 ) && !defined( __APPLE__ )
extern void         setgrent(void);
#endif

#endif /* WIN32 */

bool ReadGroupEntries()
{
#if !defined(WIN32)
  std::size_t group_count = 0;

  for (; getgrent(); ++group_count);

  setgrent();

  group_array.clear();
  group_array.reserve(group_count);

  for (std::size_t i = 0; i < group_count; ++i)
  {
    errno = 0;
    if (const auto grp_ptr = getgrent())
    {
      const std::string name(grp_ptr->gr_name);

      group_array.push_back({
        static_cast<int>(grp_ptr->gr_gid),
        name.substr(0, GROUP_NAME_MAX),
        CutName(name, DISPLAY_GROUP_NAME_MAX),
      });
    } else {
      if (errno == 0)
      {
        // Not syre why this can happen, but continue...
        break;
      }
      ERROR_MSG("Getgrent Failed");
      group_array.clear();

      return false;
    }
  }
#endif

  return true;
}

std::optional<std::string> GetGroupName(unsigned int gid)
{
#if defined(WIN32)
  const auto group_ptr = getgrgid(gid);

  return group_ptr ? group_ptr->gr_name : std::nullopt;
#else
  for (const auto& group : group_array)
  {
    if (group.gid == static_cast<int>(gid))
    {
      return group.name;
    }
  }

  return std::nullopt;
#endif
}

std::optional<std::string> GetDisplayGroupName(unsigned int gid)
{
#if defined(WIN32)
  const auto group_ptr = getgrgid(gid);

  return group_ptr ? group_ptr->gr_name : str::nullopt;
#else
  for (const auto& group : group_array)
  {
    if (group.gid == static_cast<int>(gid))
    {
      return group.display_name;
    }
  }

  return std::nullopt;
#endif
}

std::optional<int> GetGroupId(const std::string& name)
{
#if defined(WIN32)
  const auto group_ptr = getgrnam(name);

  return group_ptr ? group_ptr->gr_gid : std::nullopt;
#else
  for (const auto& group : group_array)
  {
    if (!name.compare(group.name))
    {
      return group.gid;
    }
  }

  return std::nullopt;
#endif /* WIN32 */
}

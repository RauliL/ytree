/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/passwd.c,v 1.16 2020/03/15 14:55:14 werner Exp $
 *
 * Handhabung von User-Nummern / Namen
 *
 ***************************************************************************/
#include "ytree.h"

#include <vector>

#ifdef WIN32

#include <pwd.h>

#else

struct Passwd
{
  int uid;
  std::string name;
  std::string display_name;
};

static std::vector<Passwd> passwd_array;

extern struct passwd *getpwent(void);
#if !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined( __FreeBSD__ ) && !defined( OSF1 ) && !defined( __APPLE__ )
extern void          setpwent(void);
#endif

#endif /* WIN32 */

bool ReadPasswdEntries()
{
#if !defined(WIN32)
  std::size_t passwd_count = 0;

  for (; getpwent(); ++passwd_count);

  setpwent();

  passwd_array.clear();
  passwd_array.reserve(passwd_count);

  for (std::size_t i = 0; i < passwd_count; ++i)
  {
    errno = 0;
    if (const auto pwd_ptr = getpwent())
    {
      const std::string name(pwd_ptr->pw_name);

      passwd_array.push_back({
        static_cast<int>(pwd_ptr->pw_uid),
        name.substr(0, OWNER_NAME_MAX),
        CutName(name, DISPLAY_OWNER_NAME_MAX),
      });
    } else {
      if (errno == 0)
      {
        // Not sure why this can happen, but continue...
        break;
      }
      ERROR_MSG("Getpwent Failed");
      passwd_array.clear();

      return false;
    }
  }
#endif

  return true;
}

std::optional<std::string> GetPasswdName(unsigned int uid)
{
#if defined(WIN32)
  const auto pwd_ptr = getpwuid(uid);

  return pwd_ptr ? pwd_ptr->pw_name : std::nullopt;
#else
  for (const auto& passwd : passwd_array)
  {
    if (passwd.uid == static_cast<int>(uid))
    {
      return passwd.name;
    }
  }

  return std::nullopt;
#endif /* WIN32 */
}

std::optional<std::string> GetDisplayPasswdName(unsigned int uid)
{
#if defined(WIN32)
  const auto pwd_ptr = getpwuid(uid);

  return pwd_ptr ? pwd_ptr->pw_name : std::nullopt;
#else
  for (const auto& passwd : passwd_array)
  {
    if (passwd.uid == static_cast<int>(uid))
    {
      return passwd.display_name;
    }
  }

  return std::nullopt;
#endif /* WIN32 */
}

std::optional<int> GetPasswdUid(const std::string& name)
{
#ifdef WIN32
  const auto pwd_ptr = getpwnam(name);

  return pwd_ptr ? pwd_ptr->pw_uid : std::nullopt;
#else
  for (const auto& passwd : passwd_array)
  {
    if (!name.compare(passwd.name))
    {
      return static_cast<int>(passwd.uid);
    }
  }

  return std::nullopt;
#endif /* WIN32 */
}

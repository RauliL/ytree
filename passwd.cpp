/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/passwd.c,v 1.16 2020/03/15 14:55:14 werner Exp $
 *
 * Handhabung von User-Nummern / Namen
 *
 ***************************************************************************/


#include "ytree.h"


#ifdef WIN32

#include <pwd.h>

#else

typedef struct
{
  int   uid;
  char  name[OWNER_NAME_MAX + 1];
  char  display_name[DISPLAY_OWNER_NAME_MAX + 1];
} PasswdEntry;


extern struct passwd *getpwent(void);
#if !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined( __FreeBSD__ ) && !defined( OSF1 ) && !defined( __APPLE__ )
extern void          setpwent(void);
#endif

static PasswdEntry   *passwd_array;
static unsigned int  passwd_count;

#endif /* WIN32 */


int ReadPasswdEntries(void)
{
#ifndef WIN32

  int i;
  struct passwd *pwd_ptr;


  for( passwd_count=0; getpwent(); passwd_count++ )
    ;

  setpwent();

  if( passwd_array )
  {
    free( passwd_array );
    passwd_array = NULL;
  }

  if( passwd_count == 0 )
  {
    passwd_array = NULL;
  }
  else
  {
    if( ( passwd_array = (PasswdEntry *) calloc( passwd_count,
					         sizeof( PasswdEntry )
					       ) ) == NULL )
    {
      ERROR_MSG( "Calloc Failed" );
      passwd_array = NULL;
      passwd_count = 0;
      return( 1 );
    }
  }

  for(i=0; i < (int)passwd_count; i++)
  {
    errno = 0;
    if( ( pwd_ptr = getpwent() ) != NULL )
    {
      passwd_array[i].uid = pwd_ptr->pw_uid;
      (void) strncpy( passwd_array[i].name, pwd_ptr->pw_name, OWNER_NAME_MAX );
      passwd_array[i].name[OWNER_NAME_MAX] = '\0';
      CutName(passwd_array[i].display_name, pwd_ptr->pw_name, DISPLAY_OWNER_NAME_MAX);
    }
    else
    {
      if(errno == 0)
      {
        passwd_count = i;  /* Not sure why this can happen, but continue... */
        break;
      }

      ERROR_MSG( "Getpwent Failed" );
      if( passwd_array) free( passwd_array );
      passwd_array = NULL;
      passwd_count = 0;
      return( 1 );
    }
  }

#endif /* WIN32 */

  return( 0 );
}

std::optional<std::string> GetPasswdName(unsigned int uid)
{
#if defined(WIN32)
  const auto pwd_ptr = getpwuid(uid);

  return pwd_ptr ? pwd_ptr->pw_name : std::nullopt;
#else
  for(int i = 0; i < static_cast<int>(passwd_count); ++i)
  {
    if (passwd_array[i].uid == static_cast<int>(uid))
    {
      return passwd_array[i].name;
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
  for(int i = 0; i < static_cast<int>(passwd_count); ++i)
  {
    if (passwd_array[i].uid == static_cast<int>(uid))
    {
      return passwd_array[i].display_name;
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
  for (int i = 0; i < static_cast<int>(passwd_count); ++i)
  {
    if (!name.compare(passwd_array[i].name))
    {
      return static_cast<int>(passwd_array[i].uid);
    }
  }

  return std::nullopt;
#endif /* WIN32 */
}

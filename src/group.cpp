/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/group.c,v 1.16 2020/03/15 14:55:13 werner Exp $
 *
 * Handhabung von Gruppen-Nummern / Namen
 *
 ***************************************************************************/


#include "ytree.h"

#ifdef WIN32

#include "grp.h"

#else

typedef struct
{
  int   gid;
  char  name[GROUP_NAME_MAX + 1];
  char  display_name[DISPLAY_GROUP_NAME_MAX + 1];
} GroupEntry;


extern struct group *getgrent(void);
#if !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined( __FreeBSD__ ) && !defined( OSF1 ) && !defined( __APPLE__ )
extern void         setgrent(void);
#endif

static GroupEntry   *group_array;
static unsigned int group_count;


#endif /* WIN32 */



int ReadGroupEntries(void)
{
#ifndef WIN32

  int i;
  struct group *grp_ptr;


  for( group_count=0; getgrent(); group_count++ )
    ;

  setgrent();

  if( group_array )
  {
    free( group_array );
    group_array = NULL;
  }

  if( group_count == 0 )
  {
    group_array = NULL;
  }
  else
  {
    if( ( group_array = (GroupEntry *) calloc( group_count,
					       sizeof( GroupEntry )
					     ) ) == NULL )
    {
      ERROR_MSG( "Calloc Failed" );
      group_array = NULL;
      group_count = 0;
      return( 1 );
    }
  }

  for(i=0; i < (int)group_count; i++)
  {
    errno = 0;
    if( ( grp_ptr = getgrent() ) != NULL )
    {
      group_array[i].gid = grp_ptr->gr_gid;
      (void)strncpy( group_array[i].name, grp_ptr->gr_name, GROUP_NAME_MAX );
      group_array[i].name[GROUP_NAME_MAX] = '\0';
      CutName(group_array[i].display_name, grp_ptr->gr_name, DISPLAY_GROUP_NAME_MAX);
    }
    else
    {
      if(errno == 0)
      {
	group_count = i;  /* Not sure why this can happen, but continue... */
        break;
      }

      ERROR_MSG( "Getgrent Failed" );
      if( group_array) free( group_array );
      group_array = NULL;
      group_count = 0;
      return( 1 );
    }
  }

#endif /* WIN32 */

  return( 0 );
}

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

/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/chgrp.c,v 1.13 2005/01/22 16:32:29 werner Exp $
 *
 * Change Group
 *
 ***************************************************************************/


#include "ytree.h"



static int SetDirGroup(DirEntry *de_ptr, int new_group_id);



int ChangeFileGroup(FileEntry *fe_ptr)
{
  WalkingPackage walking_package;
  int  group_id;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( group_id = GetNewGroup( fe_ptr->stat_struct.st_gid ) ) >= 0 )
  {
    walking_package.function_data.change_group.new_group_id = group_id;
    result = SetFileGroup( fe_ptr, &walking_package );
  }
  return( result );
}




int GetNewGroup(int st_gid)
{
  char group[GROUP_NAME_MAX * 2 +1];
  int  id;
  int  group_id;

  group_id = -1;

  id = ( st_gid == -1 ) ? (int) getgid() : st_gid;

  if (const auto group_name_ptr = GetGroupName(id))
  {
    std::strncpy(group, group_name_ptr->c_str(), sizeof(group));
  } else {
    std::snprintf(group, sizeof(group), "%d", id);
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "New Group:" );

  if( InputString( group, LINES - 2, 12, 0, GROUP_NAME_MAX, "\r\033" ) == CR )
  {
    if (const auto group_id_ptr = GetGroupId(group))
    {
      group_id = *group_id_ptr;
    } else {
      MessagePrintf("Can't read Group-ID:*\"%s\"", group);
    }
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( group_id );
}




int SetFileGroup(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  const auto path = GetFileNamePath(fe_ptr);
  struct stat stat_struct;
  int  result;
  int  new_group_id;

  result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  new_group_id = walking_package->function_data.change_group.new_group_id;

  if (!chown(path.c_str(), fe_ptr->stat_struct.st_uid, new_group_id))
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/

    if (STAT_(path.c_str(), &stat_struct))
    {
      ERROR_MSG("Stat Failed");
    } else {
      fe_ptr->stat_struct = stat_struct;
    }
    result = 0;
  } else {
    MessagePrintf("Can't change owner:*%s", std::strerror(errno));
  }

  return( result );
}




int ChangeDirGroup(DirEntry *de_ptr)
{
  int  group_id;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( group_id = GetNewGroup( de_ptr->stat_struct.st_gid ) ) >= 0 )
  {
    result = SetDirGroup( de_ptr, group_id );
  }
  return( result );
}






static int SetDirGroup(DirEntry *de_ptr, int new_group_id)
{
  struct stat stat_struct;
  char buffer[PATH_LENGTH+1];
  int  result;

  result = -1;


  if( !chown( GetPath( de_ptr, buffer ),
	      de_ptr->stat_struct.st_uid ,
	      new_group_id
	    ) )
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/

    if( STAT_( buffer, &stat_struct ) )
    {
      Warning( "stat failed" );
    }
    else
    {
      de_ptr->stat_struct = stat_struct;
    }
    result = 0;
  } else {
    MessagePrintf("Can't change owner:*%s", std::strerror(errno));
  }

  return( result );
}

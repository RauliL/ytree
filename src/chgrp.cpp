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

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( group_id = GetNewGroup(fe_ptr->stat_struct.st_gid) ) >= 0 )
  {
    walking_package.function_data.change_group.new_group_id = group_id;
    result = SetFileGroup(fe_ptr, &walking_package);
  }
  return( result );
}

int GetNewGroup(int st_gid)
{
  std::string group;
  const int id = st_gid == -1 ? static_cast<int>(getgid()) : st_gid;
  int group_id = -1;

  if (const auto group_name_ptr = GetGroupName(id))
  {
    group = *group_name_ptr;
  } else {
    group = std::to_string(id);
  }

  ClearHelp();

  MvAddStr(LINES - 2, 1, "New Group:");

  if (InputString(group, LINES - 2, 12, 0, GROUP_NAME_MAX) == CR)
  {
    if (const auto group_id_ptr = GetGroupId(group))
    {
      group_id = *group_id_ptr;
    } else {
      FormatMessage("Can't read Group-ID:*\"{}\"", group);
    }
  }

  move(LINES - 2, 1);
  clrtoeol();

  return group_id;
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
      Error("stat() failed");
    } else {
      fe_ptr->stat_struct = stat_struct;
    }
    result = 0;
  } else {
    FormatMessage("Can't change owner:*{}", std::strerror(errno));
  }

  return( result );
}




int ChangeDirGroup(DirEntry *de_ptr)
{
  int  group_id;
  int  result;

  result = -1;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( group_id = GetNewGroup(de_ptr->stat_struct.st_gid) ) >= 0 )
  {
    result = SetDirGroup(de_ptr, group_id);
  }
  return( result );
}

static int SetDirGroup(DirEntry *de_ptr, int new_group_id)
{
  const auto path = GetPath(de_ptr);

  if (!chown(path.c_str(), de_ptr->stat_struct.st_uid, new_group_id))
  {
    struct stat st;

    /* Erfolgreich modifiziert */
    /*-------------------------*/
    if (STAT_(path.c_str(), &st))
    {
      Warning("stat() failed");
    } else {
      de_ptr->stat_struct = st;
    }

    return 0;
  }
  FormatMessage("Can't change owner:*{}", std::strerror(errno));

  return -1;
}

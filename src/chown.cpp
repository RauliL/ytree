/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/chown.c,v 1.13 2005/01/22 16:32:29 werner Exp $
 *
 * Change Owner
 *
 ***************************************************************************/


#include "ytree.h"



static int SetDirOwner(DirEntry *de_ptr, int new_owner_id);



int ChangeFileOwner(FileEntry *fe_ptr)
{
  WalkingPackage walking_package;
  int  owner_id;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( owner_id = GetNewOwner( fe_ptr->stat_struct.st_uid ) ) >= 0 )
  {
    walking_package.function_data.change_owner.new_owner_id = owner_id;
    result = SetFileOwner( fe_ptr, &walking_package );
  }
  return( result );
}




int GetNewOwner(int st_uid)
{
  char owner[OWNER_NAME_MAX * 2 +1];
  int  owner_id;
  int  id;

  owner_id = -1;

  id = (st_uid == -1) ? (int) getuid() : st_uid;

  if (const auto owner_name_ptr = GetPasswdName(id))
  {
    std::strncpy(owner, owner_name_ptr->c_str(), sizeof(owner));
  } else {
    std::snprintf(owner, sizeof(owner), "%d", id);
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "New Owner:" );

  if (InputString( owner, LINES - 2, 12, 0, OWNER_NAME_MAX))
  {
    if (const auto owner_id_ptr = GetPasswdUid(owner))
    {
      owner_id = *owner_id_ptr;
    } else {
      MessagePrintf("Can't read Owner-ID:*%s", owner);
    }
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( owner_id );
}




int SetFileOwner(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  const auto path = GetFileNamePath(fe_ptr);
  struct stat stat_struct;
  int  result;
  int  new_owner_id;

  result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  new_owner_id = walking_package->function_data.change_owner.new_owner_id;

  if (!chown(path.c_str(), new_owner_id, fe_ptr->stat_struct.st_gid))
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/
    if (STAT_(path.c_str(), &stat_struct))
    {
      Error("stat() Failed");
    } else {
      fe_ptr->stat_struct = stat_struct;
    }
    result = 0;
  } else {
    MessagePrintf("Can't change Owner:*%s", std::strerror(errno));
  }

  return( result );
}





int ChangeDirOwner(DirEntry *de_ptr)
{
  int  owner_id;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  if( ( owner_id = GetNewOwner( de_ptr->stat_struct.st_uid ) ) >= 0 )
  {
    result = SetDirOwner( de_ptr, owner_id );
  }
  return( result );
}

static int SetDirOwner(DirEntry* de_ptr, int new_owner_id)
{
  const auto path = GetPath(de_ptr);

  if (!chown(path.c_str(), new_owner_id, de_ptr->stat_struct.st_gid))
  {
    struct stat st;

    /* Erfolgreich modifiziert */
    /*-------------------------*/
    if (STAT_(path.c_str(), &st))
    {
      Error("stat() failed");
    } else {
      de_ptr->stat_struct = st;
    }

    return 0;
  }
  MessagePrintf("Can't change Owner:*%s", std::strerror(errno));

  return -1;
}

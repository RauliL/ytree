#include "./walker.hpp"

static int SetDirOwner(DirEntry *de_ptr, int new_owner_id);

int ChangeFileOwner(FileEntry *fe_ptr)
{
  ChangeOwnerWalkContext ctx;
  int  result;

  result = -1;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  if (const auto owner = GetNewOwner(fe_ptr->stat_struct.st_uid))
  {
    if (const auto owner_id_ptr = GetPasswdUid(*owner))
    {
      ctx.new_owner_id = *owner_id_ptr;
      result = SetFileOwner(fe_ptr, &ctx);
    } else {
      FormatMessage("Can't read Owner-ID:*{}", *owner);
    }
  }
  return( result );
}




std::optional<std::string> GetNewOwner(int st_uid)
{
  std::string owner;
  const int id = (st_uid == -1) ? static_cast<int>(getuid()) : st_uid;

  if (const auto owner_name_ptr = GetPasswdName(id))
  {
    owner = *owner_name_ptr;
  } else {
    owner = std::to_string(id);
  }

  ClearHelp();

  MvAddStr(LINES - 2, 1, "New Owner:");

  const auto new_owner = InputString(owner, LINES - 2, 12, 0, OWNER_NAME_MAX);
  move(LINES - 2, 1);
  clrtoeol();
  return new_owner;
}




int SetFileOwner(FileEntry *fe_ptr, ChangeOwnerWalkContext *ctx)
{
  const auto path = GetFileNamePath(fe_ptr);
  struct stat stat_struct;
  int  result;
  int  new_owner_id;

  result = -1;

  ctx->new_fe_ptr = fe_ptr; /* unchanged */

  new_owner_id = ctx->new_owner_id;

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
    FormatMessage("Can't change Owner:*{}", std::strerror(errno));
  }

  return( result );
}





int ChangeDirOwner(DirEntry *de_ptr)
{
  int  result;

  result = -1;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  if (const auto owner = GetNewOwner(de_ptr->stat_struct.st_uid))
  {
    if (const auto owner_id_ptr = GetPasswdUid(*owner))
    {
      result = SetDirOwner(de_ptr, *owner_id_ptr);
    } else {
      FormatMessage("Can't read Owner-ID:*{}", *owner);
    }
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
  FormatMessage("Can't change Owner:*{}", std::strerror(errno));

  return -1;
}

#include "./walker.hpp"

static int SetDirGroup(DirEntry *de_ptr, int new_group_id);

int ChangeFileGroup(FileEntry *fe_ptr)
{
  ChangeGroupWalkContext ctx;
  int  result;

  result = -1;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  if (const auto group = GetNewGroup(fe_ptr->stat_struct.st_gid))
  {
    if (const auto group_id_ptr = GetGroupId(*group))
    {
      ctx.new_group_id = *group_id_ptr;
      result = SetFileGroup(fe_ptr, &ctx);
    } else {
      FormatMessage("Can't read Group-ID:*\"{}\"", *group);
    }
  }
  return( result );
}

std::optional<std::string> GetNewGroup(int st_gid)
{
  std::string group;
  const int id = st_gid == -1 ? static_cast<int>(getgid()) : st_gid;

  if (const auto group_name_ptr = GetGroupName(id))
  {
    group = *group_name_ptr;
  } else {
    group = std::to_string(id);
  }

  ClearHelp();

  MvAddStr(LINES - 2, 1, "New Group:");

  const auto new_group = InputString(group, LINES - 2, 12, 0, GROUP_NAME_MAX);
  move(LINES - 2, 1);
  clrtoeol();
  return new_group;
}

int SetFileGroup(FileEntry *fe_ptr, ChangeGroupWalkContext *ctx)
{
  const auto path = GetFileNamePath(fe_ptr);
  struct stat stat_struct;
  int  result;
  int  new_group_id;

  result = -1;

  ctx->new_fe_ptr = fe_ptr; /* unchanged */

  new_group_id = ctx->new_group_id;

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
  int  result;

  result = -1;

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( result );
  }

  if (const auto group = GetNewGroup(de_ptr->stat_struct.st_gid))
  {
    if (const auto group_id_ptr = GetGroupId(*group))
    {
      result = SetDirGroup(de_ptr, *group_id_ptr);
    } else {
      FormatMessage("Can't read Group-ID:*\"{}\"", *group);
    }
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

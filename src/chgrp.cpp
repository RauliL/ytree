#include "./walker.hpp"

static constexpr std::size_t GROUP_NAME_MAX = 64;

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

  if (const auto group = GetNewGroup(fe_ptr->meta.GidOrZero()))
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
  const auto path = fe_ptr->GetPath();
  int  result;
  int  new_group_id;

  result = -1;

  ctx->new_fe_ptr = fe_ptr; /* unchanged */

  new_group_id = ctx->new_group_id;

  if (!chown(path.c_str(), fe_ptr->meta.UidOrZero(), new_group_id))
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/

    if (auto meta = EntryMeta::FromFilesystem(path))
    {
      fe_ptr->SetMeta(*meta);
    } else {
      Error("stat() failed");
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

  if (const auto group = GetNewGroup(de_ptr->meta.GidOrZero()))
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

  if (!chown(path.c_str(), de_ptr->meta.UidOrZero(), new_group_id))
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/
    if (auto meta = EntryMeta::FromFilesystem(path))
    {
      de_ptr->SetMeta(*meta);
    } else {
      Warning("stat() failed");
    }

    return 0;
  }
  FormatMessage("Can't change owner:*{}", std::strerror(errno));

  return -1;
}

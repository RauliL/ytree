#include "./walker.hpp"

static constexpr std::size_t OWNER_NAME_MAX = 64;

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

  if (const auto owner = GetNewOwner(fe_ptr->meta.UidOrZero()))
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

int SetFileOwner(FileEntry* fe_ptr, ChangeOwnerWalkContext* ctx)
{
  const auto path = fe_ptr->GetPath();
  int result = -1;
  int new_owner_id;

  ctx->new_fe_ptr = fe_ptr; // unchanged

  new_owner_id = ctx->new_owner_id;

  if (!chown(path.c_str(), new_owner_id, fe_ptr->meta.GidOrZero()))
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/
    if (auto meta = EntryMeta::FromFilesystem(path))
    {
      fe_ptr->SetMeta(*meta);
    } else {
      Error("stat() Failed");
    }
    result = 0;
  } else {
    FormatMessage("Can't change Owner:*{}", std::strerror(errno));
  }

  return result;
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

  if (const auto owner = GetNewOwner(de_ptr->meta.UidOrZero()))
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

  if (!chown(path.c_str(), new_owner_id, de_ptr->meta.GidOrZero()))
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/
    if (auto meta = EntryMeta::FromFilesystem(path))
    {
      de_ptr->SetMeta(*meta);
    } else {
      Error("stat() failed");
    }

    return 0;
  }
  FormatMessage("Can't change Owner:*{}", std::strerror(errno));

  return -1;
}

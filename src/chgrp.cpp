#include "./ytree.hpp"

static constexpr std::size_t GROUP_NAME_MAX = 64;

static bool SetDirGroup(DirEntry* de_ptr, int new_group_id);

bool FileEntry::ChangeGroup()
{
  bool result = false;

  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    beep();

    return false;
  }

  if (const auto group = GetNewGroup(meta.GidOrZero()))
  {
    if (const auto group_id_ptr = GetGroupId(*group))
    {
      result = SetFileGroup(this, *group_id_ptr);
    } else {
      FormatMessage("Can't read Group-ID:*\"{}\"", *group);
    }
  }

  return result;
}

std::optional<std::string> GetNewGroup(int st_gid)
{
  std::string group;
  const int id = st_gid == -1 ? static_cast<int>(getgid()) : st_gid;
  std::optional<std::string> new_group;

  if (const auto group_name_ptr = GetGroupName(id))
  {
    group = *group_name_ptr;
  } else {
    group = std::to_string(id);
  }

  ClearHelp();
  MvAddStr(LINES - 2, 1, "New Group:");
  new_group = InputString(group, LINES - 2, 12, 0, GROUP_NAME_MAX);
  move(LINES - 2, 1);
  clrtoeol();

  return new_group;
}

bool SetFileGroup(FileEntry *fe_ptr, unsigned new_group_id)
{
  const auto path = fe_ptr->GetPath();

  if (!chown(
    path.c_str(),
    fe_ptr->meta.UidOrZero(),
    static_cast<int>(new_group_id)
  ))
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/

    if (auto meta = EntryMeta::FromFilesystem(path))
    {
      fe_ptr->SetMeta(*meta);
    } else {
      Error("stat() failed");
    }

    return true;
  }

  FormatMessage("Can't change owner:*{}", std::strerror(errno));

  return false;
}

bool DirEntry::ChangeGroup()
{
  bool result = false;

  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    beep();

    return false;
  }

  if (const auto group = GetNewGroup(meta.GidOrZero()))
  {
    if (const auto group_id_ptr = GetGroupId(*group))
    {
      result = SetDirGroup(this, *group_id_ptr);
    } else {
      FormatMessage("Can't read Group-ID:*\"{}\"", *group);
    }
  }

  return result;
}

static bool SetDirGroup(DirEntry* de_ptr, int new_group_id)
{
  const auto path = GetPath(de_ptr);

  if (!chown(path.c_str(), de_ptr->meta.UidOrZero(), new_group_id))
  {
    // Erfolgreich modifiziert
    if (auto meta = EntryMeta::FromFilesystem(path))
    {
      de_ptr->SetMeta(*meta);
    } else {
      Warning("stat() failed");
    }

    return true;
  }
  FormatMessage("Can't change owner:*{}", std::strerror(errno));

  return false;
}

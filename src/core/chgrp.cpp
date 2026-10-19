#include "ytree.hpp"

static constexpr std::size_t GROUP_NAME_MAX = 64;

bool Entry::ChangeGroup()
{
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    beep();

    return false;
  }

  if (const auto group = GetNewGroup(meta.GidOrZero()))
  {
    if (const auto group_id_ptr = GetGroupId(*group))
    {
      return SetGroup(*group_id_ptr);
    }
    FormatMessage("Can't read Group-ID:*\"{}\"", *group);
  }

  return false;
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

bool Entry::SetGroup(int new_group_id)
{
  const auto path = GetPath();

  if (!chown(path.c_str(), meta.UidOrZero(), new_group_id))
  {
    // Erfolgreich modifiziert
    if (const auto new_meta = EntryMeta::FromFilesystem(path))
    {
      meta = *new_meta;
    } else {
      Error("stat() failed");
    }

    return true;
  }

  FormatMessage("Can't change owner:*{}", std::strerror(errno));

  return false;
}

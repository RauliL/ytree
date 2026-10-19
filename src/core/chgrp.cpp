#include "core/ui.hpp"
#include "core/api.hpp"

static constexpr std::size_t GROUP_NAME_MAX = 64;

bool Entry::ChangeGroup()
{
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    ui::Beep();

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

  if (const auto group_name_ptr = GetGroupName(id))
  {
    group = *group_name_ptr;
  } else {
    group = std::to_string(id);
  }

  return ui::Prompt("New Group:", group, GROUP_NAME_MAX);
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
      YTreeError("stat() failed");
    }

    return true;
  }

  FormatMessage("Can't change owner:*{}", std::strerror(errno));

  return false;
}

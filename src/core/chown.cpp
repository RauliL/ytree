#include "ytree.hpp"

static constexpr std::size_t OWNER_NAME_MAX = 64;

bool Entry::ChangeOwner()
{
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    beep();

    return false;
  }

  if (const auto owner = GetNewOwner(meta.UidOrZero()))
  {
    if (const auto owner_id_ptr = GetPasswdUid(*owner))
    {
      return SetOwner(*owner_id_ptr);
    }
    FormatMessage("Can't read Owner-ID:*{}", *owner);
  }

  return false;
}

std::optional<std::string> GetNewOwner(int st_uid)
{
  std::string owner;
  const int id = (st_uid == -1) ? static_cast<int>(getuid()) : st_uid;
  std::optional<std::string> new_owner;

  if (const auto owner_name_ptr = GetPasswdName(id))
  {
    owner = *owner_name_ptr;
  } else {
    owner = std::to_string(id);
  }

  ClearHelp();
  MvAddStr(LINES - 2, 1, "New Owner:");
  new_owner = InputString(owner, LINES - 2, 12, 0, OWNER_NAME_MAX);
  move(LINES - 2, 1);
  clrtoeol();

  return new_owner;
}

bool Entry::SetOwner(int new_owner_id)
{
  const auto path = GetPath();

  if (!chown(path.c_str(), new_owner_id, meta.GidOrZero()))
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
  FormatMessage("Can't change Owner:*{}", std::strerror(errno));

  return false;
}

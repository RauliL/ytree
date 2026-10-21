#include <cstring>

#include <unistd.h>

#include "core/ui.hpp"
#include "core/api.hpp"

static constexpr std::size_t OWNER_NAME_MAX = 64;

bool Entry::ChangeOwner()
{
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    ui::Beep();

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

  if (const auto owner_name_ptr = GetPasswdName(id))
  {
    owner = *owner_name_ptr;
  } else {
    owner = std::to_string(id);
  }

  return ui::Prompt("New Owner:", owner, OWNER_NAME_MAX);
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
      YTreeError("stat() failed");
    }

    return true;
  }
  FormatMessage("Can't change Owner:*{}", std::strerror(errno));

  return false;
}

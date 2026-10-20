#include "core/ui.hpp"
#include "core/api.hpp"

static int GetNewModus(int old_modus, std::string_view modus);

bool Entry::ChangeModus()
{
  if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
  {
    auto modus = GetAttributes(meta.Mode());
    bool result = false;

    if (ui::EditFileMode(modus, "\r\033"))
    {
      result = SetModus(modus);
    }

    return result;
  }
  ui::Beep();

  return false;
}

bool GetNewFileModus(int /*y*/, int /*x*/, std::string& modus, const char *term)
{
  return ui::EditFileMode(modus, term);
}

bool Entry::SetModus(const std::string& new_modus_str)
{
  using std::filesystem::perms;

  const auto path = GetPath();
  int new_modus = GetNewModus(meta.Mode(), new_modus_str);
  std::error_code ec;

  // Keep file type and the sticky bit; replace the mode bits this editor sets.
  new_modus = new_modus | (meta.Mode() & ~static_cast<int>(
    perms::owner_all |
    perms::group_all |
    perms::others_all |
    perms::set_uid |
    perms::set_gid
  ));
  const auto permissions = static_cast<perms>(new_modus);
  std::filesystem::permissions(path, permissions, ec);
  if (!ec)
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

  FormatMessage("Cant't change modus:*{}", ec.message());

  return false;
}

static int GetNewModus(int old_modus, std::string_view modus)
{
  using std::filesystem::perms;

  const auto at = [modus](std::size_t index) -> char
  {
    return index < modus.size() ? modus[index] : '\0';
  };

  // 'on' sets that bit. '?' keeps it from old_modus.
  // set_id is used by the execute columns: 's' sets set-id together with execute,
  // and '?' keeps both.
  const auto bits_for = [old_modus](
    char symbol,
    char on,
    perms bit,
    perms set_id = perms::none
  ) -> perms
  {
    if (symbol == on)
    {
      return bit;
    }
    else if (symbol == 's' && set_id != perms::none)
    {
      return set_id | bit;
    }
    else if (symbol == '?')
    {
      return static_cast<perms>(old_modus & static_cast<int>(set_id | bit));
    }

    return perms::none;
  };

  perms permissions = perms::none;

  // ls-style mode after the type character: rwx for user, group, then other.
  permissions |= bits_for(at(1), 'r', perms::owner_read);
  permissions |= bits_for(at(2), 'w', perms::owner_write);
  permissions |= bits_for(at(3), 'x', perms::owner_exec, perms::set_uid);
  permissions |= bits_for(at(4), 'r', perms::group_read);
  permissions |= bits_for(at(5), 'w', perms::group_write);
  permissions |= bits_for(at(6), 'x', perms::group_exec, perms::set_gid);
  permissions |= bits_for(at(7), 'r', perms::others_read);
  permissions |= bits_for(at(8), 'w', perms::others_write);
  permissions |= bits_for(at(9), 'x', perms::others_exec);

  // File type is not part of std::filesystem::perms. Callers still expect
  // those bits in the combined mode value.
  int new_modus = static_cast<int>(permissions);

  switch (at(0))
  {
    case '-':
      new_modus |= S_IFREG;
      break;

    case 'd':
      new_modus |= S_IFDIR;
      break;

#ifdef S_IFLNK
    case 'l':
      new_modus |= S_IFLNK;
      break;
#endif

    case '?':
      new_modus |= old_modus & S_IFMT;
      break;

    default:
      break;
  }

  return new_modus;
}

int GetModus(std::string_view modus)
{
  return GetNewModus(0, modus);
}

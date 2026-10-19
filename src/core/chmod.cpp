#include "core/ui.hpp"
#include "ytree.hpp"

static int GetNewModus(int old_modus, const std::string& new_modus);

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
  const auto path = GetPath();
  int new_modus = GetNewModus(meta.Mode(), new_modus_str);
  std::error_code ec;
  std::filesystem::perms perms;

  new_modus = new_modus | (meta.Mode() & ~(
    S_IRWXO |
    S_IRWXG |
    S_IRWXU |
    S_ISGID |
    S_ISUID
  ));
  perms = static_cast<std::filesystem::perms>(new_modus);
  std::filesystem::permissions(path, perms, ec);
  if (!ec)
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

  FormatMessage("Cant't change modus:*{}", ec.message());

  return false;
}

static int GetNewModus(int old_modus, const std::string& modus_str)
{
  int new_modus;
  const char *modus = modus_str.c_str();

  new_modus = 0;

  if( *modus == '-' ) new_modus |= S_IFREG;
  if( *modus == 'd' ) new_modus |= S_IFDIR;
#ifdef S_IFLNK
  if( *modus == 'l' ) new_modus |= S_IFLNK;
#endif /* S_IFLNK */
  if( *modus == '?' ) new_modus |= old_modus & S_IFMT;
  modus++;

  if( *modus   == 'r' ) new_modus |= S_IRUSR;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IRUSR;
  if( *modus   == 'w' ) new_modus |= S_IWUSR;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IWUSR;
  if( *modus   == 'x' ) new_modus |= S_IXUSR;
  if( *modus   == 's' ) new_modus |= S_ISUID | S_IXUSR;
  if( *modus++ == '?' ) new_modus |= old_modus & (S_ISUID | S_IXUSR);

  if( *modus   == 'r' ) new_modus |= S_IRGRP;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IRGRP;
  if( *modus   == 'w' ) new_modus |= S_IWGRP;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IWGRP;
  if( *modus   == 'x' ) new_modus |= S_IXGRP;
  if( *modus   == 's' ) new_modus |= S_ISGID | S_IXGRP;
  if( *modus++ == '?' ) new_modus |= old_modus & (S_ISGID | S_IXGRP);

  if( *modus   == 'r' ) new_modus |= S_IROTH;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IROTH;
  if( *modus   == 'w' ) new_modus |= S_IWOTH;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IWOTH;
  if( *modus   == 'x' ) new_modus |= S_IXOTH;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IXOTH;

  return( new_modus );
}

int GetModus(const std::string& modus)
{
  return GetNewModus(0, modus);
}

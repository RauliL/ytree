#include <functional>

#include "ytree.hpp"

static int GetNewModus(int old_modus, const std::string& new_modus);

bool Entry::ChangeModus()
{
  if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
  {
    auto modus = GetAttributes(meta.Mode());
    bool result = false;

    if (GetNewFileModus(LINES - 2, 1, modus, "\r\033"))
    {
      result = SetModus(modus);
    }

    move(LINES - 2, 1);
    clrtoeol();

    return result;
  }
  beep();

  return false;
}

bool GetNewFileModus(int y, int x, std::string& modus, const char *term)
{
  int c, p;
  static char rwx[] = "rwx";

  ClearHelp();
  curs_set(1);
  MvAddStr(y, x, "New Filemodus:");

  x += 16;

  p = 0;
  MvAddStr(y, x, modus);
  leaveok(stdscr, false);
  do
  {
    move(y, x + p);
    RefreshWindow(stdscr);
    doupdate();

    c = Getch();

#ifdef VI_KEYS
    c = ViKey(c);
#endif /* VI_KEYS */

    if( c == LF ) c = CR;

    if( p > 0 && ( c == '?' || c == '-' || c == rwx[(p-1) % 3] ) )
    {
      /* gueltige Eingabe */
      /*------------------*/

      modus[p] = (char) c;
      addch(c);
      if( p < 9 ) p++;
    }
    else if( c == 's' && ( p == 3 || p == 6 ) )
    {
      /* Set-ID */
      /*--------*/

      if( modus[p] != 'x' && modus[p] != 's' )
      {
  Message("Execute-Permission required*for set-ID");
      }
      else
      {
        modus[p] = (char) c;
        addch(c);
        if( p < 9 ) p++;
      }
    }
    else
    {
      if( c == ' ' && p < 9 ) p++;
      else if( c == KEY_LEFT && p > 0 ) p--;
      else if( c == KEY_RIGHT && p < 9 ) p++;
      else if( std::strrchr(term, c) == nullptr ) beep();
    }
  } while( c != -1 && std::strrchr(term, c) == nullptr );
  leaveok(stdscr, true);
  move(y, x); clrtoeol();
  curs_set(0);

  return c == CR;
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

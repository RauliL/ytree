/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/chmod.c,v 1.14 2001/06/15 16:36:36 werner Exp $
 *
 * Change Modus
 *
 ***************************************************************************/


#include "ytree.h"



static int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package);
static int GetNewModus(int old_modus, const char *new_modus );



int ChangeFileModus(FileEntry *fe_ptr)
{
  char modus[11];
  WalkingPackage walking_package;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  (void) GetAttributes( fe_ptr->stat_struct.st_mode, modus );

  if( GetNewFileModus( LINES - 2, 1, modus, "\r\033" ) == CR )
  {
    (void) strcpy( walking_package.function_data.change_modus.new_modus, modus );
    result = SetFileModus( fe_ptr, &walking_package );
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}





int ChangeDirModus(DirEntry *de_ptr)
{
  char modus[11];
  WalkingPackage walking_package;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  (void) GetAttributes( de_ptr->stat_struct.st_mode, modus );

  if( GetNewFileModus( LINES - 2, 1, modus, "\r\033" ) == CR )
  {
    (void) strcpy( walking_package.function_data.change_modus.new_modus, modus );
    result = SetDirModus( de_ptr, &walking_package );
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}




int GetNewFileModus(int y, int x, char *modus, const char *term)
{
  int c, p;
  static char rwx[] = "rwx";

  ClearHelp();
  curs_set(1);
  MvAddStr( y, x, "New Filemodus:" );

  x += 16;

  p = 0;
  MvAddStr(y, x, modus );
  leaveok(stdscr, false);
  do
  {
    move( y, x + p );
    RefreshWindow( stdscr );
    doupdate();

    c = Getch();

#ifdef VI_KEYS
    c = ViKey( c );
#endif /* VI_KEYS */

    if( c == LF ) c = CR;

    if( p > 0 && ( c == '?' || c == '-' || c == rwx[(p-1) % 3] ) )
    {
      /* gueltige Eingabe */
      /*------------------*/

      modus[p] = (char) c;
      addch( c );
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
        addch( c );
        if( p < 9 ) p++;
      }
    }
    else
    {
      if( c == ' ' && p < 9 ) p++;
      else if( c == KEY_LEFT && p > 0 ) p--;
      else if( c == KEY_RIGHT && p < 9 ) p++;
      else if( strrchr( term, c ) == NULL ) beep();
    }
  } while( c != -1 && strrchr( term, c ) == NULL );
  leaveok(stdscr, true);
  move( y, x ); clrtoeol();
  curs_set(0);

  return( c );
}




int SetFileModus(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  const auto path = GetFileNamePath(fe_ptr);
  struct stat stat_struct;
  int  result;
  int  new_modus;

  result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  new_modus = GetNewModus( fe_ptr->stat_struct.st_mode,
			   walking_package->function_data.change_modus.new_modus
			 );

  new_modus = new_modus | ( fe_ptr->stat_struct.st_mode &
	      ~( S_IRWXO | S_IRWXG | S_IRWXU | S_ISGID | S_ISUID ) );

  if (!chmod(path.c_str(), new_modus))
  {
    /* Erfolgreich modifiziert */
    /*-------------------------*/
    if (STAT_(path.c_str(), &stat_struct))
    {
      Error("stat() failed");
    } else {
      fe_ptr->stat_struct = stat_struct;
    }

    result = 0;
  } else {
    MessagePrintf("Cant't change modus:*%s", std::strerror(errno));
  }

  return( result );
}

static int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package)
{
  const auto path = GetPath(de_ptr);
  auto new_modus = GetNewModus(
    de_ptr->stat_struct.st_mode,
    walking_package->function_data.change_modus.new_modus
  );

  new_modus = new_modus | (de_ptr->stat_struct.st_mode & ~(
    S_IRWXO | S_IRWXG | S_IRWXU | S_ISGID | S_ISUID
  ));

  if (!chmod(path.c_str(), new_modus))
  {
    struct stat st;

    /* Erfolgreich modifiziert */
    /*-------------------------*/
    if (STAT_(path.c_str(), &st))
    {
      Error("stat() Failed");
    } else {
      de_ptr->stat_struct = st;
    }

    return 0;
  }
  MessagePrintf("Cant't change modus:*%s", std::strerror(errno));

  return -1;
}

static int GetNewModus(int old_modus, const char *modus )
{
  int new_modus;

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




int GetModus(const char *modus)
{
  return( GetNewModus( 0, modus ) );
}






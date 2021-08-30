#include "ytree.h"

static bool RenameDirEntry(const std::string&, const std::string&);
static bool RenameFileEntry(const std::string&, const std::string&);

int RenameDirectory(DirEntry *de_ptr, char *new_name)
{
  DirEntry    *den_ptr;
  DirEntry    *sde_ptr;
  DirEntry    *ude_ptr;
  FileEntry   *fe_ptr;
  const auto from_path = GetPath(de_ptr);
  char        to_path[PATH_LENGTH+1];
  struct stat stat_struct;
  int         result;
  char        *cptr;

  result = -1;

  std::strcpy(to_path, from_path.c_str());
  cptr = std::strrchr(to_path, '/');

  if (!cptr)
  {
    WarningPrintf("Invalid Path!*\"%s\"", to_path);
    ESCAPE;
  }

  if( cptr == to_path )
  {
    MESSAGE( "Can't rename ROOT" );
    ESCAPE;
  }

  (void) strcpy( cptr + 1, new_name );

  if (!IsWriteable(from_path))
  {
    MessagePrintf(
      "Rename not possible!*\"%s\"*%s",
      from_path.c_str(),
      std::strerror(errno)
    );
    ESCAPE;
  }



  if (RenameDirEntry(to_path, from_path))
  {
    /* Rename erfolgreich */
    /*--------------------*/
    StatOrAbort(to_path, stat_struct);
    den_ptr = MallocOrAbort<DirEntry>(sizeof(DirEntry) + std::strlen(new_name));

    (void) memcpy( den_ptr, de_ptr, sizeof( DirEntry ) );

    (void) strcpy( den_ptr->name, new_name );

    (void) memcpy( &den_ptr->stat_struct,
		   &stat_struct,
		   sizeof( stat_struct )
		 );

    /* Struktur einklinken */
    /*---------------------*/

    if( den_ptr->prev ) den_ptr->prev->next = den_ptr;
    if( den_ptr->next ) den_ptr->next->prev = den_ptr;

    /* Subtree */
    /*---------*/

    for( sde_ptr=den_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next )
      sde_ptr->up_tree = den_ptr;

    /* Files */
    /*-------*/

    for( fe_ptr=den_ptr->file; fe_ptr; fe_ptr=fe_ptr->next )
      fe_ptr->dir_entry = den_ptr;

    /* Uptree */
    /*--------*/

    for( ude_ptr=den_ptr->up_tree; ude_ptr; ude_ptr = ude_ptr->next )
      if( ude_ptr->sub_tree == de_ptr ) ude_ptr->sub_tree = den_ptr;

    /* Alte Struktur freigeben */
    /*-------------------------*/

    free( de_ptr );

    /* Achtung: de_ptr ist ab jetzt ungueltig !!! */
    /*--------------------------------------------*/

    result = 0;
  }

FNC_XIT:

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}






int RenameFile(FileEntry *fe_ptr, char *new_name, FileEntry **new_fe_ptr )
{
  FileEntry   *fen_ptr;
  const auto de_ptr = fe_ptr->dir_entry;
  const auto from_path = GetFileNamePath(fe_ptr);
  const auto to_path = GetPath(de_ptr) + FILE_SEPARATOR_CHAR + new_name;
  struct stat stat_struct;
  int         result;

  result = -1;

  *new_fe_ptr = fe_ptr;

  if (!IsWriteable(from_path))
  {
    MessagePrintf(
      "Rename not possible!*\"%s\"*%s",
      from_path.c_str(),
      std::strerror(errno)
    );

    return -1;
  }

  if (RenameFileEntry(to_path, from_path))
  {
    /* Rename erfolgreich */
    /*--------------------*/
    StatOrAbort(to_path, stat_struct);
    fen_ptr = MallocOrAbort<FileEntry>(sizeof(FileEntry) + std::strlen(new_name));

    (void) memcpy( fen_ptr, fe_ptr, sizeof( FileEntry ) );

    (void) strcpy( fen_ptr->name, new_name );

    (void) memcpy( &fen_ptr->stat_struct,
		   &stat_struct,
		   sizeof( stat_struct )
		 );

    /* Struktur einklinken */
    /*---------------------*/

    if( fen_ptr->prev ) fen_ptr->prev->next = fen_ptr;
    if( fen_ptr->next ) fen_ptr->next->prev = fen_ptr;
    if( fen_ptr->dir_entry->file == fe_ptr ) fen_ptr->dir_entry->file = fen_ptr;

    /* Alte Struktur freigeben */
    /*-------------------------*/

    free( fe_ptr );

    /* Achtung: fe_ptr ist ab jetzt ungueltig !!! */
    /*--------------------------------------------*/

    result = 0;

    *new_fe_ptr = fen_ptr;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}





int GetRenameParameter(char *old_name, char *new_name)
{
  int l;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( -1 );
  }

  ClearHelp();

  if( old_name == NULL )
  {
    MvAddStr( LINES - 2, 1, "RENAME TAGGED FILES TO:" );
    l = 25;
  }
  else
  {
    MvAddStr( LINES - 2, 1, "RENAME TO:" );
    l = 13;
  }

  (void) strcpy( new_name, (old_name) ? old_name : "*" );


  if( InputString(new_name, LINES - 2, l, 0, COLS - l - 1, "\r\033" ) != CR)
    return( -1 );

  if(!strlen(new_name))
    return( -1 );

  if(old_name && !strcmp(old_name, new_name))
  {
    MESSAGE("Can't rename: New name same as old name.");
    return( -1 );
  }

  if(strrchr(new_name, FILE_SEPARATOR_CHAR) != NULL)
  {
    MESSAGE("Invalid new name:*No slashes when renaming!");
    return( -1 );
  }

  return( 0 );
}

static bool RenameDirEntry(
  const std::string& to_path,
  const std::string& from_path
)
{
  if (!to_path.compare(from_path))
  {
    Message("Can't rename directory:*New Name == Old Name");

    return false;
  }

  if (Exists(to_path))
  {
    Message("Can't rename directory:*Destination object already exist!");

    return false;
  }

  if (rename(from_path.c_str(), to_path.c_str()))
  {
    MessagePrintf(
      "Can't rename \"%s\"*to \"%s\"*%s",
      from_path.c_str(),
      to_path.c_str(),
      std::strerror(errno)
    );

    return false;
  }

  return true;
}

static bool RenameFileEntry(
  const std::string& to_path,
  const std::string& from_path)
{
  if (!to_path.compare(from_path))
  {
    Message("Can't rename!*New Name == Old Name");

    return false;
  }

  if (link(from_path.c_str(), to_path.c_str()))
  {
    MessagePrintf(
      "Can't link \"%s\"*to \"%s\"*%s",
      from_path.c_str(),
      to_path.c_str(),
      std::strerror(errno)
    );

    return false;
  }

  if (unlink(from_path.c_str()))
  {
    MessagePrintf(
      "Can't unlink*\"%s\"*%s",
      from_path.c_str(),
      std::strerror(errno)
    );

    return false;
  }

  return true;
}

int RenameTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  int  result = -1;
  char new_name[PATH_LENGTH+1];


  if( BuildFilename( fe_ptr->name,
                     walking_package->function_data.rename.new_name,
		     new_name
		   ) == 0 )
  {
    if( *new_name == '\0' )
    {
      MESSAGE( "Can't rename file to*empty name" );
    }
    else
    {
      result = RenameFile( fe_ptr, new_name, &walking_package->new_fe_ptr );
    }
  }
  return( result );
}


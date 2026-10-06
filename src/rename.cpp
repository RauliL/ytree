#include "ytree.h"

#include <filesystem>
#include <system_error>

static bool RenameDirEntry(const std::string&, const std::string&);
static bool RenameFileEntry(const std::string&, const std::string&);

int RenameDirectory(DirEntry *de_ptr, char *new_name)
{
  DirEntry    *den_ptr;
  DirEntry    *sde_ptr;
  DirEntry    *ude_ptr;
  FileEntry   *fe_ptr;
  const auto from_path = GetPath(de_ptr);
  const std::filesystem::path from_fs_path(from_path);
  std::string to_path;
  struct stat stat_struct;
  int         result;

  result = -1;

  if (!from_fs_path.has_filename() ||
      from_fs_path == std::filesystem::path(
        std::string_view(&std::filesystem::path::preferred_separator, 1)))
  {
    Message("Can't rename ROOT");
    ESCAPE;
  }

  to_path = (from_fs_path.parent_path() / new_name).string();

  if (!IsWriteable(from_path))
  {
    FormatMessage("Rename not possible!*\"{}\"*{}", from_path.c_str(), std::strerror(errno));
    ESCAPE;
  }



  if (RenameDirEntry(to_path, from_path))
  {
    /* Rename erfolgreich */
    /*--------------------*/
    StatOrAbort(to_path, stat_struct);
    den_ptr = NewOrAbort<DirEntry>(*de_ptr);
    den_ptr->name = new_name;
    den_ptr->stat_struct = stat_struct;

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

    delete de_ptr;

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
  const auto to_path =
    (std::filesystem::path(GetPath(de_ptr)) / new_name).string();
  struct stat stat_struct;
  int         result;

  result = -1;

  *new_fe_ptr = fe_ptr;

  if (!IsWriteable(from_path))
  {
    FormatMessage("Rename not possible!*\"{}\"*{}", from_path.c_str(), std::strerror(errno));

    return -1;
  }

  if (RenameFileEntry(to_path, from_path))
  {
    /* Rename erfolgreich */
    /*--------------------*/
    StatOrAbort(to_path, stat_struct);
    fen_ptr = NewOrAbort<FileEntry>(*fe_ptr);
    fen_ptr->name = new_name;
    fen_ptr->stat_struct = stat_struct;

    /* Struktur einklinken */
    /*---------------------*/

    if( fen_ptr->prev ) fen_ptr->prev->next = fen_ptr;
    if( fen_ptr->next ) fen_ptr->next->prev = fen_ptr;
    if( fen_ptr->dir_entry->file == fe_ptr ) fen_ptr->dir_entry->file = fen_ptr;

    /* Alte Struktur freigeben */
    /*-------------------------*/

    delete fe_ptr;

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

  if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
  {
    beep();
    return( -1 );
  }

  ClearHelp();

  if( old_name == nullptr )
  {
    MvAddStr( LINES - 2, 1, "RENAME TAGGED FILES TO:" );
    l = 25;
  }
  else
  {
    MvAddStr( LINES - 2, 1, "RENAME TO:" );
    l = 13;
  }

  *std::format_to(new_name, "{}", (old_name) ? old_name : "*") = '\0';


  if (InputString(new_name, LINES - 2, l, 0, COLS - l - 1) != CR)
  {
    return -1;
  }

  if(!strlen(new_name))
    return( -1 );

  if (old_name && !std::strcmp(old_name, new_name))
  {
    Message("Can't rename: New name same as old name.");

    return -1;
  }

  if (std::strrchr(new_name, std::filesystem::path::preferred_separator))
  {
    Message("Invalid new name:*No slashes when renaming!");

    return -1;
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

  std::error_code ec;
  std::filesystem::rename(from_path, to_path, ec);
  if (ec)
  {
    FormatMessage("Can't rename \"{}\"*to \"{}\"*{}", from_path.c_str(), to_path.c_str(), ec.message().c_str());

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

  if (Exists(to_path))
  {
    Message("Can't rename!*Destination object already exist!");

    return false;
  }

  std::error_code ec;
  std::filesystem::rename(from_path, to_path, ec);
  if (ec)
  {
    FormatMessage("Can't rename \"{}\"*to \"{}\"*{}", from_path.c_str(), to_path.c_str(), ec.message().c_str());

    return false;
  }

  return true;
}

int RenameTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  int  result = -1;
  char new_name[PATH_LENGTH+1];


  if( BuildFilename( fe_ptr->name.data(),
                     walking_package->function_data.rename.new_name,
		     new_name
		   ) == 0 )
  {
    if( *new_name == '\0' )
    {
      Message("Can't rename file to*empty name");
    }
    else
    {
      result = RenameFile( fe_ptr, new_name, &walking_package->new_fe_ptr );
    }
  }
  return( result );
}


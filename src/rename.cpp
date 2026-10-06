#include "ytree.h"

#include <filesystem>
#include <system_error>

static bool RenameDirEntry(const std::string&, const std::string&);
static bool RenameFileEntry(const std::string&, const std::string&);

int RenameDirectory(DirEntry *de_ptr, const std::string& new_name)
{
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

    /* The entry is owned by shared_ptr and only holds weak back-references,
     * so it can be renamed in place: parent, children, files, statistic.tree
     * and disk_statistic.tree all stay valid.
     */
    de_ptr->name = new_name;
    de_ptr->stat_struct = stat_struct;

    result = 0;
  }

FNC_XIT:

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}






int RenameFile(FileEntry *fe_ptr, const std::string& new_name, FileEntry **new_fe_ptr )
{
  const auto de_ptr = fe_ptr->Dir();
  const auto from_path = GetFileNamePath(fe_ptr);
  const auto to_path =
    (std::filesystem::path(GetPath(de_ptr.get())) / new_name).string();
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

    /* Rename in place; see RenameDirectory(). */
    fe_ptr->name = new_name;
    fe_ptr->stat_struct = stat_struct;

    result = 0;

  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}





int GetRenameParameter(const std::string* old_name, char *new_name)
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

  *std::format_to(new_name, "{}", old_name ? *old_name : "*") = '\0';


  if (InputString(new_name, LINES - 2, l, 0, COLS - l - 1) != CR)
  {
    return -1;
  }

  if(!strlen(new_name))
    return( -1 );

  if (old_name && *old_name == new_name)
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


  if( BuildFilename( fe_ptr->name,
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


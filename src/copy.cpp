#include "ytree.h"

#include <filesystem>
#include <system_error>

static int Copy(const std::string& to_path, const std::string& from_path);
static int CopyArchiveFile(const std::string& to_path, const std::string& from_path);

int CopyFile(Statistic *statistic_ptr,
             FileEntry *fe_ptr,
             bool confirm,
             char *to_file,
             DirEntry *dest_dir_entry,
             char *to_dir_path,       /* absoluter Pfad */
             bool path_copy
	    )
{
  long long file_size;
  const auto from_path = GetRealFileNamePath(fe_ptr);
  const auto from_dir = GetPath(fe_ptr->dir_entry);
  std::filesystem::path to_fs_path;
  std::string to_path;
  char        buffer[20];
  FileEntry   *dest_file_entry;
  FileEntry   *fen_ptr;
  struct stat stat_struct;
  int         term;
  int         result;
  int	      refresh_dirwindow = false;


  result = -1;

  if (std::strcmp(to_dir_path, FILE_SEPARATOR_STRING))
  {
    /* not ROOT */
    /*----------*/

    to_fs_path = to_dir_path;
  }
  if (path_copy)
  {
    const auto path = std::filesystem::path(GetPath(fe_ptr->dir_entry));

    /* Create destination folder (if neccessary) */
    /*-------------------------------------------*/
    if (to_fs_path.empty())
    {
      to_fs_path = path;
    }
    else
    {
      /* strcat-style append: avoid absolute-path replacement */
      to_fs_path /= path.is_absolute() ? path.relative_path() : path;
    }

    if (!to_fs_path.is_absolute())
    {
      to_fs_path = std::filesystem::path(from_dir) / to_fs_path;
    }

    to_path = to_fs_path.string();
    if (MakePath(statistic_ptr->tree, to_path.data(), &dest_dir_entry))
    {
      MessagePrintf(
        "Can't create path*\"%s\"*%s",
        to_path.c_str(),
        std::strerror(errno)
      );

      return result;
    }
  }
  {
    std::error_code ec;
    const auto dir_path = to_fs_path.empty()
      ? std::filesystem::path(FILE_SEPARATOR_STRING)
      : to_fs_path;

    if (!std::filesystem::is_directory(dir_path, ec) &&
        !std::filesystem::exists(dir_path, ec))
    {
      if ((term = InputChoise(
            "Directory does not exist; create (y/N) ? ",
            "YN\033"
          )) == 'Y')
      {
        if (!to_fs_path.empty() && !to_fs_path.is_absolute())
        {
          to_fs_path = std::filesystem::path(from_dir) / to_fs_path;
        }
        to_path = to_fs_path.string();
        if (MakePath(statistic_ptr->tree, to_path.data(), &dest_dir_entry))
        {
          MessagePrintf(
            "Can't create path*\"%s\"*%s",
            to_path.c_str(),
            std::strerror(errno)
          );

          return result;
        }
        else
        {
          refresh_dirwindow = true;
        }
      }
      else
      {
        return result;
      }
    }
  }
  if (to_fs_path.empty())
  {
    to_fs_path = FILE_SEPARATOR_STRING;
  }
  to_fs_path /= to_file;
  to_path = to_fs_path.string();


#ifdef DEBUG
  fprintf( stderr, "Copy: \"%s\" --> \"%s\"\n", from_path.c_str(), to_path.c_str() );
#endif /* DEBUG */

  if (to_path == from_path)
  {
    Message("Can't copy file into itself");

    return result;
  }


  if( dest_dir_entry )
  {
    /* Ziel befindet sich im Sub-Tree */
    /*--------------------------------*/

    (void) GetFileEntry( dest_dir_entry, to_file, &dest_file_entry );

    if( dest_file_entry )
    {
      /* Datei existiert */
      /*-----------------*/

      if( confirm )
      {
	term = InputChoise( "file exist; overwrite (Y/N) ? ", "YN\033" );

        if( term != 'Y' )
        {
	  result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
          ESCAPE;
        }
      }

      (void) DeleteFile( dest_file_entry );
    }
  }
  /* access benutzen */
  /*-----------------*/
  else if (Exists(to_path) && confirm)
  {
    /* Datei existiert */
    /*-----------------*/
    term = InputChoise( "file exist; overwrite (Y/N) ? ", "YN\033" );
    if (term != 'Y')
    {
      result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
      ESCAPE;
    }
  }


  if( !Copy( to_path, from_path ) )
  {
    /* File wurde kopiert */
    /*--------------------*/

    if( chmod( to_path.c_str(), fe_ptr->stat_struct.st_mode ) == -1 )
    {
      WarningPrintf(
        "Can't chmod file*\"%s\"*to mode %s*IGNORED",
        to_path.c_str(),
        GetAttributes(fe_ptr->stat_struct.st_mode, buffer)
      );
    }

    if( dest_dir_entry )
    {
      StatOrAbort(to_path, stat_struct);

      file_size = stat_struct.st_size;

      dest_dir_entry->total_bytes += file_size;
      dest_dir_entry->total_files++;
      statistic_ptr->disk_total_bytes += file_size;
      statistic_ptr->disk_total_files++;
      dest_dir_entry->matching_bytes += file_size;
      dest_dir_entry->matching_files++;
      statistic_ptr->disk_matching_bytes += file_size;
      statistic_ptr->disk_matching_files++;

      /* File eintragen */
      /*----------------*/
      fen_ptr = MallocOrAbort<FileEntry>(sizeof(FileEntry) + std::strlen(to_file));

      (void) strcpy( fen_ptr->name, to_file );

      (void) memcpy( &fen_ptr->stat_struct,
		     &stat_struct,
		     sizeof( stat_struct )
		   );

      fen_ptr->dir_entry   = dest_dir_entry;
      fen_ptr->tagged      = false;
      fen_ptr->matching    = Match( fen_ptr->name );
      fen_ptr->next        = dest_dir_entry->file;
      fen_ptr->prev        = nullptr;
      if( dest_dir_entry->file ) dest_dir_entry->file->prev = fen_ptr;
      dest_dir_entry->file = fen_ptr;
    }

    (void) GetAvailBytes( &statistic_ptr->disk_space );

    result = 0;
  }

  if( refresh_dirwindow)
  {
  	RefreshDirWindow();
  }

FNC_XIT:

  move( LINES - 3, 1 ); clrtoeol();
  move( LINES - 2, 1 ); clrtoeol();
  move( LINES - 1, 1 ); clrtoeol();

  return( result );
}





int GetCopyParameter(const char *from_file, bool path_copy, char *to_file, char *to_dir)
{
  char buffer[PATH_LENGTH + 1];

  if( from_file == nullptr )
  {
    from_file = "TAGGED FILES";
    (void) strcpy( to_file, "*" );
  }
  else
  {
    (void) strcpy( to_file, from_file );
  }

  if( path_copy )
  {
    std::snprintf(buffer, sizeof(buffer), "PATHCOPY %s", from_file);
  } else {
    std::snprintf(buffer, sizeof(buffer), "COPY %s", from_file);
  }

  ClearHelp();

  MvAddStr( LINES - 3, 1, buffer );
  MvAddStr( LINES - 2, 1, "AS   ");

  if (InputString(to_file, LINES - 2, 6, 0, COLS - 6) == CR)
  {
    MvAddStr( LINES - 1, 1, "TO   " );
    if (InputString( to_dir, LINES - 1, 6, 0, COLS - 6) == CR)
    {
      return 0;
    }
  }
  ClearHelp();

  return -1;
}

static int Copy(const std::string& to_path, const std::string& from_path)
{
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    return CopyArchiveFile(to_path, from_path);
  }

#ifdef DEBUG
  fprintf( stderr, "Copy: \"%s\" --> \"%s\"\n", from_path.c_str(), to_path.c_str() );
#endif /* DEBUG */

  if (!to_path.compare(from_path))
  {
    Message("Can't copy file into itself");

    return -1;
  }

  std::error_code ec;
  std::filesystem::copy_file(
    from_path,
    to_path,
    std::filesystem::copy_options::overwrite_existing,
    ec
  );
  if (ec)
  {
    MessagePrintf(
      "Can't copy file*\"%s\"*to*\"%s\"*%s",
      from_path.c_str(),
      to_path.c_str(),
      ec.message().c_str()
    );

    return -1;
  }

  return 0;
}

int CopyTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  char new_name[PATH_LENGTH+1];
  int  result = -1;

  walking_package->new_fe_ptr = fe_ptr;  /* unchanged */

  if( BuildFilename( fe_ptr->name,
		     walking_package->function_data.copy.to_file,
		     new_name
		   ) == 0 )
  {
    if( *new_name == '\0' )
    {
      Message("Can't copy file to*empty name");
    }

    result = CopyFile( walking_package->function_data.copy.statistic_ptr,
		       fe_ptr,
		       walking_package->function_data.copy.confirm,
		       new_name,
		       walking_package->function_data.copy.dest_dir_entry,
		       walking_package->function_data.copy.to_path,
		       walking_package->function_data.copy.path_copy
		     );
  }

  return( result );
}

static int CopyArchiveFile(
  const std::string& to_path,
  const std::string& from_path
)
{
  const auto from_p_aux = ShellEscape(to_path);
  const auto to_p_aux = ShellEscape(from_path);
  const auto command_line = MakeExtractCommandLine(
    mode == Mode::TAPE_MODE ? statistic.tape_name : statistic.login_path,
    from_p_aux,
    "> \"" + to_p_aux + "\""
  );
  const auto result = SilentSystemCall(command_line);

  if (result)
  {
    WarningPrintf(
      "Can't copy file*%s*to file*%s",
      from_p_aux.c_str(),
      to_p_aux.c_str()
    );
  }

  return result;
}

#include "ytree.h"

static bool Move(const std::string&, const std::string&);

int MoveFile(FileEntry *fe_ptr,
       bool confirm,
       char *to_file,
       DirEntry *dest_dir_entry,
       char *to_dir_path,
       FileEntry **new_fe_ptr
)
{
  const auto de_ptr = fe_ptr->Dir();
  const auto from_path =
    (std::filesystem::path(GetPath(de_ptr.get())) / fe_ptr->name).string();
  std::int64_t file_size;
  char        to_path[PATH_LENGTH+1];
  FileEntry   *dest_file_entry;
  struct stat stat_struct;
  int         term;
  int         result;

  result = -1;
  *new_fe_ptr = nullptr;

  const auto to_path_str =
    (std::filesystem::path(to_dir_path) / to_file).string();
  std::strncpy(to_path, to_path_str.c_str(), PATH_LENGTH);
  to_path[PATH_LENGTH] = '\0';

  if (!std::strcmp(to_path, from_path.c_str()))
  {
    Message("Can't move file into itself");
    ESCAPE;
  }

  if (!IsWriteable(from_path))
  {
    FormatMessage("Unmoveable file*\"{}\"*{}", from_path.c_str(), std::strerror(errno));
    ESCAPE;
  }


  if( dest_dir_entry )
  {
    /* Ziel befindet sich im Sub-Tree */
    /*--------------------------------*/

    GetFileEntry(dest_dir_entry, to_file, &dest_file_entry);

    if( dest_file_entry )
    {
      /* Datei existiert */
      /*-----------------*/

      if( confirm )
      {
  term = InputChoise("file exist; overwrite (Y/N) ? ", "YN\033");

        if( term != 'Y' ) {
    result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
    ESCAPE;
  }
      }

      DeleteFile(dest_file_entry);
    }
  }
  /* access benutzen */
  /*-----------------*/
  else if (std::filesystem::exists(to_path))
  {
    /* Datei existiert */
    /*-----------------*/

    if( confirm )
    {
      term = InputChoise("file exist; overwrite (Y/N) ? ", "YN\033");
      if (term != 'Y')
      {
        result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
        ESCAPE;
      }
    }

    if (unlink(to_path))
    {
      FormatMessage("Can't unlink*\"{}\"*{}", to_path, std::strerror(errno));
      ESCAPE;
    }
  }


  if (Move(to_path, from_path))
  {
    /* File wurde bewegt */
    /*-------------------*/

    /* Original aus Baum austragen */
    /*-----------------------------*/

    RemoveFile(fe_ptr);


    if( dest_dir_entry )
    {
      StatOrAbort(to_path, stat_struct);

      file_size = stat_struct.st_size;

      dest_dir_entry->total_bytes += file_size;
      dest_dir_entry->total_files++;
      statistic.disk_total_bytes += file_size;
      statistic.disk_total_files++;
      dest_dir_entry->matching_bytes += file_size;
      dest_dir_entry->matching_files++;
      statistic.disk_matching_bytes += file_size;
      statistic.disk_matching_files++;

      /* File eintragen */
      /*----------------*/
      auto fen_ptr = std::make_shared<FileEntry>();
      fen_ptr->name = to_file;
      fen_ptr->stat_struct = stat_struct;
      fen_ptr->dir_entry   = dest_dir_entry->weak_from_this();
      fen_ptr->matching    = Match(fen_ptr->name);
      dest_dir_entry->files.insert(dest_dir_entry->files.begin(), fen_ptr);
      *new_fe_ptr          = fen_ptr.get();
    }

    GetAvailBytes(&statistic.disk_space);

    result = 0;
  }

FNC_XIT:

  move(LINES - 3, 1); clrtoeol();
  move(LINES - 2, 1); clrtoeol();
  move(LINES - 1, 1); clrtoeol();

  return( result );
}





int GetMoveParameter(const char *from_file, char *to_file, char *to_dir)
{
  char buffer[PATH_LENGTH * 2 +1];
  std::string to_file_str;
  std::string to_dir_str;

  if( from_file == nullptr )
  {
    from_file = "TAGGED FILES";
    to_file_str = "*";
  }
  else
  {
    to_file_str = from_file;
  }

  std::snprintf(buffer, sizeof(buffer), "MOVE %s", from_file);

  ClearHelp();

  MvAddStr(LINES - 3, 1, buffer);
  MvAddStr(LINES - 2, 1, "AS  ");
  if (InputString(to_file_str, LINES - 2, 6, 0, COLS - 6) == CR)
  {
    MvAddStr(LINES - 1, 1, "TO   ");
    if (InputString(to_dir_str, LINES - 1, 6, 0, COLS - 6) == CR)
    {
      *std::format_to(to_file, "{}", to_file_str) = '\0';
      *std::format_to(to_dir, "{}", to_dir_str) = '\0';
      return 0;
    }
  }
  ClearHelp();

  return -1;
}

static bool Move(const std::string& to_path, const std::string& from_path)
{
  if (!to_path.compare(from_path))
  {
    Message("Can't move file into itself");

    return false;
  }

  if (link(from_path.c_str(), to_path.c_str()))
  {
    FormatMessage("Can't link \"{}\"*to \"{}\"*{}", from_path.c_str(), to_path.c_str(), std::strerror(errno));

    return false;
  }

  if (unlink(from_path.c_str()))
  {
    FormatMessage("Can't unlink*\"{}\"*{}", from_path.c_str(), std::strerror(errno));

    return false;
  }

  return true;
}

int MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  int  result = -1;
  char new_name[PATH_LENGTH+1];


  if( BuildFilename(fe_ptr->name,
                     walking_package->function_data.mv.to_file,
         new_name
) == 0 )

  {
    if (!*new_name)
    {
      Message("Can't move file to*empty name");
    }
    else
    {
      result = MoveFile(fe_ptr,
             walking_package->function_data.mv.confirm,
             new_name,
             walking_package->function_data.mv.dest_dir_entry,
             walking_package->function_data.mv.to_path,
             &walking_package->new_fe_ptr
);
    }
  }

  return( result );
}


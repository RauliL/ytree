#include "ytree.h"

static bool Move(const std::string&, const std::string&);

int MoveFile(FileEntry *fe_ptr,
	     unsigned char confirm,
	     char *to_file,
	     DirEntry *dest_dir_entry,
	     char *to_dir_path,
	     FileEntry **new_fe_ptr
	    )
{
  const auto de_ptr = fe_ptr->dir_entry;
  const auto from_path = GetPath(de_ptr) + FILE_SEPARATOR_CHAR + fe_ptr->name;
  long long file_size;
  char        to_path[PATH_LENGTH+1];
  FileEntry   *dest_file_entry;
  FileEntry   *fen_ptr;
  struct stat stat_struct;
  int         term;
  int         result;

  result = -1;
  *new_fe_ptr = NULL;

  (void) strcpy( to_path, to_dir_path );
  (void) strcat( to_path, FILE_SEPARATOR_STRING );
  (void) strcat( to_path, to_file );

  if (!std::strcmp(to_path, from_path.c_str()))
  {
    Message("Can't move file into itself");
    ESCAPE;
  }

  if (!IsWriteable(from_path))
  {
    MessagePrintf(
      "Unmoveable file*\"%s\"*%s",
      from_path.c_str(),
      std::strerror(errno)
    );
    ESCAPE;
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

        if( term != 'Y' ) {
	  result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
	  ESCAPE;
	}
      }

      (void) DeleteFile( dest_file_entry );
    }
  }
  /* access benutzen */
  /*-----------------*/
  else if (Exists(to_path))
  {
    /* Datei existiert */
    /*-----------------*/

    if( confirm )
    {
      term = InputChoise( "file exist; overwrite (Y/N) ? ", "YN\033" );
      if (term != 'Y')
      {
        result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
        ESCAPE;
      }
    }

    if (unlink(to_path))
    {
      MessagePrintf(
        "Can't unlink*\"%s\"*%s",
        to_path,
        std::strerror(errno)
      );
      ESCAPE;
    }
  }


  if (Move(to_path, from_path))
  {
    /* File wurde bewegt */
    /*-------------------*/

    /* Original aus Baum austragen */
    /*-----------------------------*/

    (void) RemoveFile( fe_ptr );


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
      fen_ptr->prev        = NULL;
      if( dest_dir_entry->file ) dest_dir_entry->file->prev = fen_ptr;
      dest_dir_entry->file = fen_ptr;
      *new_fe_ptr          = fen_ptr;
    }

    (void) GetAvailBytes( &statistic.disk_space );

    result = 0;
  }

FNC_XIT:

  move( LINES - 3, 1 ); clrtoeol();
  move( LINES - 2, 1 ); clrtoeol();
  move( LINES - 1, 1 ); clrtoeol();

  return( result );
}





int GetMoveParameter(const char *from_file, char *to_file, char *to_dir)
{
  char buffer[PATH_LENGTH * 2 +1];

  if( from_file == NULL )
  {
    from_file = "TAGGED FILES";
    (void) strcpy( to_file, "*" );
  }
  else
  {
    (void) strcpy( to_file, from_file );
  }

  (void) sprintf( buffer, "MOVE %s", from_file );

  ClearHelp();

  MvAddStr( LINES - 3, 1, buffer );
  MvAddStr( LINES - 2, 1, "AS  " );
  if (InputString( to_file, LINES - 2, 6, 0, COLS - 6) == CR)
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

static bool Move(const std::string& to_path, const std::string& from_path)
{
  if (!to_path.compare(from_path))
  {
    Message("Can't move file into itself");

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

int MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  int  result = -1;
  char new_name[PATH_LENGTH+1];


  if( BuildFilename( fe_ptr->name,
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
      result = MoveFile( fe_ptr,
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


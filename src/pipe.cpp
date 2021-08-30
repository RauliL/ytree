#include "ytree.h"

int Pipe(DirEntry* dir_entry, FileEntry* file_entry)
{
  static char input_buffer[COMMAND_LINE_LENGTH + 1] = "| ";
  char file_name_path[PATH_LENGTH + 1];
  std::string file_name_p_aux;
  auto command_line = MallocOrAbort<char>(COMMAND_LINE_LENGTH + 1);
  char path[PATH_LENGTH + 1];
  int result = -1;

  result = -1;

  GetRealFileNamePath(file_entry, file_name_path);
  file_name_p_aux = ShellEscape(file_name_path);

  ClearHelp();

  MvAddStr(LINES - 2, 1, "Pipe-Command:");
  if (!GetPipeCommand(&input_buffer[2]))
  {
    move(LINES - 2, 1);
    clrtoeol();

    GetPath(dir_entry, path);

    if (mode == DISK_MODE || mode == USER_MODE)
    {
      /* Kommandozeile zusammenbasteln */
      /*-------------------------------*/
      std::snprintf(
        command_line,
        COMMAND_LINE_LENGTH,
        "%s %s %s",
        CAT,
        file_name_p_aux.c_str(),
        input_buffer
      );
    } else {
      /* TAR/ZOO/ZIP_FILE_MODE */
      /*-----------------------*/
      MakeExtractCommandLine(
        command_line,
        COMMAND_LINE_LENGTH,
        mode == TAPE_MODE ? statistic.tape_name : statistic.login_path,
        file_name_p_aux,
			  input_buffer
			);
    }
    refresh();
    result = QuerySystemCall(command_line);
  } else {
    move(LINES - 2, 1);
    clrtoeol();
  }

  std::free(static_cast<void*>(command_line));

  return result;
}





int GetPipeCommand(char *pipe_command)
{
  int  result;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Pipe-Command: " );
  if( InputString( pipe_command, LINES - 2, 15, 0, COLS - 16, "\r\033" ) == CR )
  {
    result = 0;
  }
  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}






int PipeTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  int  i, n;
  char from_path[PATH_LENGTH+1];
  char buffer[2048];


  walking_package->new_fe_ptr = fe_ptr;  /* unchanged */

  (void) GetRealFileNamePath( fe_ptr, from_path );
  if ((i = open(from_path, O_RDONLY)) == -1)
  {
    MessagePrintf(
      "Can't open file*\"%s\"*%s",
      from_path,
      std::strerror(errno)
    );

    return -1;
  }

  while( ( n = read( i, buffer, sizeof( buffer ) ) ) > 0 )
  {
    if( fwrite( buffer,
		n,
		1,
		walking_package->function_data.pipe_cmd.pipe_file ) != 1
      )
    {
      MessagePrintf("Write-Error!*%s", std::strerror(errno));
      close(i);

      return -1;
    }
  }

  (void) close( i );

  return( 0 );
}

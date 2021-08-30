#include "ytree.h"

int Pipe(DirEntry* dir_entry, FileEntry* file_entry)
{
  static char input_buffer[COMMAND_LINE_LENGTH + 1] = "| ";
  const auto file_name_path = GetRealFileNamePath(file_entry);
  const auto file_name_p_aux = ShellEscape(file_name_path);
  std::string command_line;
  int result = -1;

  ClearHelp();

  MvAddStr(LINES - 2, 1, "Pipe-Command:");
  if (!GetPipeCommand(&input_buffer[2]))
  {
    move(LINES - 2, 1);
    clrtoeol();

    if (mode == DISK_MODE || mode == USER_MODE)
    {
      /* Kommandozeile zusammenbasteln */
      /*-------------------------------*/
      command_line = std::string(CAT) + " \"" + file_name_p_aux + "\" " + input_buffer;
    } else {
      /* TAR/ZOO/ZIP_FILE_MODE */
      /*-----------------------*/
      command_line = MakeExtractCommandLine(
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

  return result;
}





int GetPipeCommand(char *pipe_command)
{
  int  result;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Pipe-Command: " );
  if (InputString( pipe_command, LINES - 2, 15, 0, COLS - 16) == CR)
  {
    result = 0;
  }
  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}

int PipeTaggedFiles(FileEntry* fe_ptr, WalkingPackage* walking_package)
{
  const auto from_path = GetRealFileNamePath(fe_ptr);
  int i;
  int n;
  char buffer[2048];

  walking_package->new_fe_ptr = fe_ptr; // Unchanged.

  if ((i = open(from_path.c_str(), O_RDONLY)) == -1)
  {
    MessagePrintf(
      "Can't open file*\"%s\"*%s",
      from_path.c_str(),
      std::strerror(errno)
    );

    return -1;
  }

  while ((n = read(i, buffer, sizeof(buffer))) > 0)
  {
    if (std::fwrite(
      buffer,
      n,
      1,
      walking_package->function_data.pipe_cmd.pipe_file
    ) != 1)
    {
      MessagePrintf("Write-Error!*%s", std::strerror(errno));
      close(i);

      return -1;
    }
  }

  close(i);

  return 0;
}

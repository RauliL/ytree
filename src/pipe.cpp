#include "./walker.hpp"

int Pipe(DirEntry* dir_entry, FileEntry* file_entry)
{
  std::string input_buffer;
  const auto file_name_path = GetRealFileNamePath(file_entry);
  std::string command_line;
  int result = -1;

  ClearHelp();

  MvAddStr(LINES - 2, 1, "Pipe-Command:");
  if (GetPipeCommand(input_buffer))
  {
    input_buffer.insert(0, "| ");
    move(LINES - 2, 1);
    clrtoeol();

    if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
    {
      /* Kommandozeile zusammenbasteln */
      /*-------------------------------*/
      command_line = Join(
        GetProfileValueOrEmpty("CAT"),
        ShellQuote(file_name_path.string()),
        input_buffer
      );
    } else {
      /* TAR/ZOO/Mode::ZIP_FILE_MODE */
      /*-----------------------*/
      command_line = MakeExtractCommandLine(
        mode == Mode::TAPE_MODE ? statistic.tape_name : statistic.login_path,
        file_name_path.string(),
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

bool GetPipeCommand(std::string& pipe_command)
{
  bool result = false;

  ClearHelp();

  MvAddStr(LINES - 2, 1, "Pipe-Command: ");
  if (InputString(pipe_command, LINES - 2, 15, 0, COLS - 16) == CR)
  {
    result = true;
  }
  move(LINES - 2, 1);
  clrtoeol();

  return result;
}

int PipeTaggedFiles(FileEntry* fe_ptr, PipeWalkContext* ctx)
{
  const auto from_path = GetRealFileNamePath(fe_ptr);
  int i;
  int n;
  char buffer[2048];

  ctx->new_fe_ptr = fe_ptr; // Unchanged.

  if ((i = open(from_path.c_str(), O_RDONLY)) == -1)
  {
    FormatMessage("Can't open file*\"{}\"*{}", from_path.c_str(), std::strerror(errno));

    return -1;
  }

  while ((n = read(i, buffer, sizeof(buffer))) > 0)
  {
    if (std::fwrite(buffer, n, 1, ctx->pipe_file) != 1)
    {
      FormatMessage("Write-Error!*{}", std::strerror(errno));
      close(i);

      return -1;
    }
  }

  close(i);

  return 0;
}

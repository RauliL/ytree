#include "ytree.hpp"

bool FileEntry::Pipe() const
{
  std::string input_buffer;

  ClearHelp();
  MvAddStr(LINES - 2, 1, "Pipe-Command:");
  if (const auto command = GetPipeCommand(input_buffer))
  {
    const auto file_name_path = GetRealPath();
    std::string command_line;

    input_buffer = *command;
    input_buffer.insert(0, "| ");
    move(LINES - 2, 1);
    clrtoeol();

    if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
    {
      // Kommandozeile zusammenbasteln
      command_line = Join(
        GetProfileValueOrEmpty("CAT"),
        ShellQuote(file_name_path.string()),
        input_buffer
      );
    } else {
      // TAR/ZOO/Mode::ZIP_FILE_MODE
      command_line = MakeExtractCommandLine(
        mode == Mode::TAPE_MODE ? statistic.tape_name : statistic.login_path,
        file_name_path.string(),
        input_buffer
      );
    }
    refresh();

    return QuerySystemCall(command_line);
  }

  move(LINES - 2, 1);
  clrtoeol();

  return false;
}

std::optional<std::string> GetPipeCommand(const std::string& initial)
{
  std::optional<std::string> command;

  ClearHelp();
  MvAddStr(LINES - 2, 1, "Pipe-Command: ");
  command = InputString(initial, LINES - 2, 15, 0, COLS - 16);
  move(LINES - 2, 1);
  clrtoeol();

  return command;
}

bool FileEntry::PipeTo(FILE* pipe_file) const
{
  const auto from_path = GetRealPath();
  int i;
  int n;
  char buffer[2048];

  if ((i = open(from_path.c_str(), O_RDONLY)) == -1)
  {
    FormatMessage("Can't open file*\"{}\"*{}", from_path.c_str(), std::strerror(errno));

    return false;
  }

  while ((n = read(i, buffer, sizeof(buffer))) > 0)
  {
    if (std::fwrite(buffer, n, 1, pipe_file) != 1)
    {
      FormatMessage("Write-Error!*{}", std::strerror(errno));
      close(i);

      return false;
    }
  }

  close(i);

  return true;
}

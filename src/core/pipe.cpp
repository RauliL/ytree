#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "core/api.hpp"
#include "core/profile.hpp"
#include "core/ui.hpp"
#include "util/string.hpp"

bool FileEntry::Pipe() const
{
  std::string input_buffer;

  if (const auto command = GetPipeCommand(input_buffer))
  {
    const auto file_name_path = GetRealPath();
    std::string command_line;

    input_buffer = *command;
    input_buffer.insert(0, "| ");

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

    return QuerySystemCall(command_line);
  }

  return false;
}

std::optional<std::string> GetPipeCommand(const std::string& initial)
{
  return ui::Prompt("Pipe-Command:", initial);
}

bool FileEntry::PipeTo(FILE* pipe_file) const
{
  const auto from_path = GetRealPath();
  int i;
  int n;
  char buffer[2048];

  if ((i = open(from_path.c_str(), O_RDONLY)) == -1)
  {
    FormatMessage(
      "Can't open file*\"{}\"*{}",
      from_path.string(),
      std::strerror(errno)
    );

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

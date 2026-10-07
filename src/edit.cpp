#include "ytree.h"

int Edit(const DirEntry* dir_entry, const std::string& file_path)
{
  std::string command_line;
  int result = -1;

  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    beep();

    return -1;
  }

  if (!IsReadable(file_path))
  {
    FormatMessage("Edit not possible!*\"{}\"*{}", file_path.c_str(), std::strerror(errno));

    return -1;
  }

  command_line = std::string(EDITOR) + " " + ShellQuote(file_path);

  if (mode == Mode::DISK_MODE)
  {
    const auto cwd = GetcwdOrDot();
    const auto path = GetPath(dir_entry);
    std::error_code ec;

    std::filesystem::current_path(path, ec);
    if (ec)
    {
      FormatMessage("Can't change directory to*\"{}\"", path.c_str());
    } else {
      result = SystemCall(command_line);
    }
    std::filesystem::current_path(cwd, ec);
    if (ec)
    {
      FormatMessage("Can't change directory to*\"{}\"", cwd.c_str());
    }
  } else {
    result = SystemCall(command_line);
  }

  return result;
}

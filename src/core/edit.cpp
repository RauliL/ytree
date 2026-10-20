#include <cstring>

#include "core/ui.hpp"
#include "core/api.hpp"

bool Edit(const DirEntry* dir_entry, const std::string& file_path)
{
  std::string command_line;
  bool result = false;

  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    ui::Beep();

    return false;
  }

  if (!IsReadable(file_path))
  {
    FormatMessage(
      "Edit not possible!*\"{}\"*{}",
      file_path,
      std::strerror(errno)
    );

    return false;
  }

  command_line = Join(GetProfileValueOrEmpty("EDITOR"), ShellQuote(file_path));

  if (mode == Mode::DISK_MODE)
  {
    const auto cwd = GetcwdOrDot();
    const auto path = GetPath(dir_entry);
    std::error_code ec;

    std::filesystem::current_path(path, ec);
    if (ec)
    {
      FormatMessage("Can't change directory to*\"{}\"", path.string());
    } else {
      result = SystemCall(command_line);
    }
    std::filesystem::current_path(cwd, ec);
    if (ec)
    {
      FormatMessage("Can't change directory to*\"{}\"", cwd.string());
    }
  } else {
    result = SystemCall(command_line);
  }

  return result;
}

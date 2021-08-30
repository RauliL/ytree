#include "ytree.h"

int Edit(const DirEntry* dir_entry, const std::string& file_path)
{
  std::string command_line;
  int result = -1;

  if (mode != DISK_MODE && mode != USER_MODE)
  {
    beep();

    return -1;
  }

  if (!IsReadable(file_path))
  {
    MessagePrintf(
		  "Edit not possible!*\"%s\"*%s",
		  file_path.c_str(),
		  std::strerror(errno)
		);

    return -1;
  }

  command_line = std::string(EDITOR) + " \"" + ShellEscape(file_path) + "\"";

  if (mode == DISK_MODE)
  {
    const auto cwd = GetcwdOrDot();
    const auto path = GetPath(dir_entry);

    if (chdir(path.c_str()))
    {
      MessagePrintf("Can't change directory to*\"%s\"", path.c_str());
    } else {
      result = SystemCall(command_line);
    }
    if (chdir(cwd.c_str()))
    {
      MessagePrintf("Can't change directory to*\"%s\"", cwd.c_str());
    }
  } else {
    result = SystemCall(command_line);
  }

  return result;
}

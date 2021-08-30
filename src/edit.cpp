#include "ytree.h"

int Edit(const DirEntry* dir_entry, const std::string& file_path)
{
  std::string command_line;
  int result;

  if (mode != DISK_MODE && mode != USER_MODE)
  {
    beep();

    return -1;
  }

  if (access(file_path.c_str(), R_OK))
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
    char path[PATH_LENGTH + 1];

    if (chdir(GetPath(dir_entry, path)))
    {
      MessagePrintf("Can't change directory to*\"%s\"", path);
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

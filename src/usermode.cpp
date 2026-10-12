#include "ytree.h"

using UserActionLookup = std::optional<std::string> (*)(int chkey, int* pchremap);

static int RunUserMode(
  int ch,
  UserActionLookup lookup,
  const std::filesystem::path& filepath
)
{
  int chremap = 0;

  while (const auto aux = lookup(ch, &chremap))
  {
    std::string command_line;

    if (const auto pos = aux->find("%s"); pos != std::string::npos)
    {
      command_line = *aux;
      command_line.replace(pos, 2, filepath.string());
    } else {
      command_line = Join(*aux, filepath.string());
    }
    if (SilentSystemCall(command_line))
    {
      FormatMessage("Can't execute*{}", command_line);
    }
    if (chremap == ch || chremap == 0)
    {
      break;
    }
    ch = chremap;
  }

  return ch;
}

int DirUserMode(DirEntry *dir_entry, int ch)
{
  return RunUserMode(ch, GetUserDirAction, GetPath(dir_entry));
}

int FileUserMode(FileEntry* file_entry, int ch)
{
  return RunUserMode(ch, GetUserFileAction, GetRealFileNamePath(file_entry));
}

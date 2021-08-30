/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/usermode.c,v 1.3 2003/08/31 11:11:00 werner Exp $
 *
 * Funktionen zur Handhabung des FILE-Windows
 *
 ***************************************************************************/


#include "ytree.h"


#define MAX( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

int DirUserMode(DirEntry *dir_entry, int ch)
{
  int chremap;
  const auto filepath = GetPath(dir_entry);
  char* command_line = nullptr;

  while (auto aux = GetUserDirAction(ch, &chremap))
  {
    if (!command_line)
    {
      command_line = MallocOrAbort<char>(COMMAND_LINE_LENGTH + 1);
    }
    if (std::strstr(aux, "%s"))
    {
      std::snprintf(command_line, COMMAND_LINE_LENGTH, aux, filepath.c_str());
    } else {
      std::snprintf(
        command_line,
        COMMAND_LINE_LENGTH,
        "%s%c%s",
        aux,
        ' ',
        filepath.c_str()
      );
    }
    if (SilentSystemCall(command_line))
    {
      MessagePrintf("Can't execute*%s", command_line);
    }
    if (chremap == ch || chremap == 0)
    {
      break;
    }
    ch = chremap;
  }

  if (command_line)
  {
    std::free(static_cast<void*>(command_line));
  }

  return ch;
}

int FileUserMode(FileEntry* file_entry, int ch)
{
  const auto filepath = GetRealFileNamePath(file_entry);
  char* command_line = nullptr;
  int chremap;

  while (const auto aux = GetUserFileAction(ch, &chremap))
  {
    if (!command_line)
    {
      command_line = MallocOrAbort<char>(COMMAND_LINE_LENGTH + 1);
    }
    if (std::strstr(aux, "%s"))
    {
      std::snprintf(
        command_line,
        COMMAND_LINE_LENGTH,
        aux,
        filepath.c_str()
      );
    } else {
      std::snprintf(
        command_line,
        COMMAND_LINE_LENGTH,
        "%s%c%s",
        aux,
        ' ',
        filepath.c_str()
      );
    }
    if (SilentSystemCall(command_line))
    {
      MessagePrintf("Can't execute*%s", command_line);
    }
    if (chremap == ch || chremap == 0)
    {
      break;
    }
    ch = chremap;
  }

  if (command_line)
  {
    std::free(static_cast<void*>(command_line));
  }

  return chremap;
}

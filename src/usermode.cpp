/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/usermode.c,v 1.3 2003/08/31 11:11:00 werner Exp $
 *
 * Funktionen zur Handhabung des FILE-Windows
 *
 ***************************************************************************/


#include "ytree.h"


int DirUserMode(DirEntry *dir_entry, int ch)
{
  int chremap;
  const auto filepath = GetPath(dir_entry);

  while (const auto aux = GetUserDirAction(ch, &chremap))
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

int FileUserMode(FileEntry* file_entry, int ch)
{
  const auto filepath = GetRealFileNamePath(file_entry);
  int chremap;

  while (const auto aux = GetUserFileAction(ch, &chremap))
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

  return chremap;
}

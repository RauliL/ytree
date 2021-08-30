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
  char *command_line, *aux;
  char filepath[PATH_LENGTH +1];

  GetPath( dir_entry, filepath );
  command_line = NULL;

  while (( aux = GetUserDirAction(ch, &chremap)) != NULL)
  {
    if (!command_line)
    {
      command_line = MallocOrAbort<char>(COMMAND_LINE_LENGTH + 1);
    }
     if (strstr(aux,"%s") != NULL) {
  	(void) sprintf( command_line, aux, filepath );
     } else {
  	(void) sprintf( command_line, "%s%c%s", aux, ' ', filepath );
     }
     if (SilentSystemCall(command_line))
     {
       MessagePrintf("can't execute*%s", command_line);
     }
     if (chremap == ch || chremap == 0)
       break;
     else
       ch = chremap;
  }

  if (command_line != NULL)
     free( command_line );

  return(ch);
}

int FileUserMode(FileEntryList* file_entry_list, int ch)
{
  const auto filepath = GetRealFileNamePath(file_entry_list->file);
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

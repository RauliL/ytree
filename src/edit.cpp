/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/edit.c,v 1.15 2014/12/26 09:53:11 werner Exp $
 *
 * Edit-Kommando-Bearbeitung
 *
 ***************************************************************************/


#include "ytree.h"



int Edit(DirEntry * dir_entry, char *file_path)
{
  std::string command_line;
  int  result = -1;
  char *file_p_aux=NULL;
  char cwd[PATH_LENGTH + 1];
  char path[PATH_LENGTH + 1];


  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( -1 );
  }

  if( access( file_path, R_OK ) )
  {
    MessagePrintf(
		  "Edit not possible!*\"%s\"*%s",
		  file_path,
		  std::strerror(errno)
		);
    ESCAPE;
  }

  file_p_aux = MallocOrAbort<char>(COMMAND_LINE_LENGTH + 1);
  StrCp(file_p_aux, file_path);

  command_line = std::string(EDITOR) + " \"" + file_p_aux + "\"";
  std::free(static_cast<void*>(file_p_aux));

  /*  result = SystemCall(command_line);
    --crb3 29apr02: perhaps finally eliminate the problem with jstar writing new
    files to the ytree starting cwd. new code grabbed from execute.c.
    --crb3 01oct02: move Getcwd operation within the IF DISKMODE stuff.
  */

  if (mode == DISK_MODE)
  {
    if (Getcwd(cwd, PATH_LENGTH) == NULL)
    {
            WARNING("Getcwd failed*\".\"assumed");
            (void) strcpy(cwd, ".");
    }

    if (chdir(GetPath(dir_entry, path)))
    {
      MessagePrintf("Can't change directory to*\"%s\"", path);
    } else {
      result = SystemCall(command_line);
    }
    if (chdir(cwd))
    {
      MessagePrintf("Can't change directory to*\"%s\"", cwd);
    }
  } else {
    result = SystemCall(command_line);
  }

FNC_XIT:

  return( result );
}

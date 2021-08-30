#include "ytree.h"

int Execute(const DirEntry* dir_entry, const FileEntry* file_entry)
{
  static char command_line[COMMAND_LINE_LENGTH + 1];
  int result = -1;

  if (file_entry && (file_entry->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
  {
    std::strcpy(command_line, ("\"" + ShellEscape(file_entry->name) + "\"").c_str());
  }

  MvAddStr(LINES - 2, 1, "Command:");
  if (!GetCommandLine(command_line))
  {
    const auto cwd = GetcwdOrDot();

    if (mode == DISK_MODE || mode == USER_MODE)
    {
      const auto path = GetPath(dir_entry);

      if (chdir(path.c_str()))
      {
        MessagePrintf("Can't change directory to*\"%s\"", path.c_str());
      } else {
        refresh();
        result = QuerySystemCall( command_line );
      }
      if (chdir(cwd.c_str()))
      {
        MessagePrintf("Can't change directory to*\"%s\"", cwd.c_str());
      }
    } else {
      refresh();
      result = QuerySystemCall( command_line );
    }
  }

  return result;
}

int GetCommandLine(char *command_line)
{
  int result;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Command: " );
  if (InputString( command_line, LINES - 2, 10, 0, COLS - 11) == CR)
  {
    move( LINES - 2, 1 ); clrtoeol();
    result = 0;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}



int GetSearchCommandLine(char *command_line)
{
  int  result;
  int  pos;
  char *cptr;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Search untag command: " );
  strcpy( command_line, SEARCHCOMMAND );

  cptr = strstr( command_line, "{}" );
  if(cptr) {
    pos = (cptr - command_line) - 1;
    if(pos < 0)
      pos = 0;
  } else {
    pos = 0;
  }
  if (InputString( command_line, LINES - 2, 23, pos, COLS - 24) == CR)
  {
    move( LINES - 2, 1 ); clrtoeol();
    result = 0;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}

int ExecuteCommand(FileEntry* fe_ptr, WalkingPackage* walking_package)
{
  std::string command_line;

  walking_package->new_fe_ptr = fe_ptr;

  for (int i = 0; walking_package->function_data.execute.command[i]; ++i)
  {
    const auto c = walking_package->function_data.execute.command[i];

    if (c == '{' && walking_package->function_data.execute.command[i + 1] == '}')
    {
      command_line += GetFileNamePath(fe_ptr);
      ++i;
    } else {
      command_line.append(1, static_cast<char>(c));
    }
  }

  return SilentSystemCallEx(command_line, false);
}

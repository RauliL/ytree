#include "ytree.h"

int Execute(const DirEntry* dir_entry, const FileEntry* file_entry)
{
  static char command_line[COMMAND_LINE_LENGTH + 1];
  int result = -1;

  if (file_entry && (file_entry->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
  {
    *std::format_to_n(
      command_line,
      COMMAND_LINE_LENGTH,
      "{}",
      ShellQuote(file_entry->name)
    ).out = '\0';
  }

  MvAddStr(LINES - 2, 1, "Command:");
  if (!GetCommandLine(command_line))
  {
    const auto cwd = GetcwdOrDot();

    if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
    {
      const auto path = GetPath(dir_entry);
      std::error_code ec;

      std::filesystem::current_path(path, ec);
      if (ec)
      {
        FormatMessage("Can't change directory to*\"{}\"", path.c_str());
      } else {
        refresh();
        result = QuerySystemCall(command_line);
      }
      std::filesystem::current_path(cwd, ec);
      if (ec)
      {
        FormatMessage("Can't change directory to*\"{}\"", cwd.c_str());
      }
    } else {
      refresh();
      result = QuerySystemCall(command_line);
    }
  }

  return result;
}

int GetCommandLine(char* command_line)
{
  int result = -1;
  std::string command;

  ClearHelp();

  MvAddStr(LINES - 2, 1, "Command: ");
  if (InputString(command, LINES - 2, 10, 0, COLS - 11) == CR)
  {
    *std::format_to(command_line, "{}", command) = '\0';
    move(LINES - 2, 1); clrtoeol();
    result = 0;
  }

  move(LINES - 2, 1); clrtoeol();

  return result;
}



int GetSearchCommandLine(char *command_line)
{
  int  result;
  int  pos;

  result = -1;

  ClearHelp();

  MvAddStr(LINES - 2, 1, "Search untag command: ");
  std::string command = GetProfileValueOrEmpty("SEARCHCOMMAND");

  if (const auto placeholder = command.find("{}"); placeholder != std::string::npos)
  {
    pos = static_cast<int>(placeholder) - 1;
    if(pos < 0)
      pos = 0;
  } else {
    pos = 0;
  }
  if (InputString(command, LINES - 2, 23, pos, COLS - 24) == CR)
  {
    *std::format_to_n(command_line, COMMAND_LINE_LENGTH, "{}", command).out = '\0';
    move(LINES - 2, 1); clrtoeol();
    result = 0;
  }

  move(LINES - 2, 1); clrtoeol();

  return( result );
}

int ExecuteCommand(FileEntry* fe_ptr, ExecuteWalkContext* ctx)
{
  std::string command_line;

  ctx->new_fe_ptr = fe_ptr;

  for (std::size_t i = 0; i < ctx->command.length(); ++i)
  {
    const auto c = ctx->command[i];

    if (c == '{' && i + 1 < ctx->command.length() && ctx->command[i + 1] == '}')
    {
      command_line += GetFileNamePath(fe_ptr);
      ++i;
    } else {
      command_line.append(1, static_cast<char>(c));
    }
  }

  return SilentSystemCallEx(command_line, false);
}

#include "./walker.hpp"

int Execute(const DirEntry* dir_entry, const FileEntry* file_entry)
{
  std::string command_line;
  int result = -1;

  if (file_entry && (file_entry->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
  {
    command_line = ShellQuote(file_entry->name);
  }

  MvAddStr(LINES - 2, 1, "Command:");
  if (const auto command = GetCommandLine(command_line))
  {
    command_line = *command;
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
        result = QuerySystemCall(command_line);
      }
      std::filesystem::current_path(cwd, ec);
      if (ec)
      {
        FormatMessage("Can't change directory to*\"{}\"", cwd.c_str());
      }
    } else {
      result = QuerySystemCall(command_line);
    }
  }

  return result;
}

std::optional<std::string> GetCommandLine(const std::string& initial)
{
  ClearHelp();

  MvAddStr(LINES - 2, 1, "Command: ");
  const auto command = InputString(initial, LINES - 2, 10, 0, COLS - 11);
  move(LINES - 2, 1);
  clrtoeol();
  return command;
}

std::optional<std::string> GetSearchCommandLine()
{
  auto command = GetProfileValueOrEmpty("SEARCHCOMMAND");
  int pos;

  ClearHelp();

  MvAddStr(LINES - 2, 1, "Search untag command: ");
  if (
    const auto placeholder = command.find("{}");
    placeholder != std::string::npos
  )
  {
    pos = static_cast<int>(placeholder) - 1;
    if (pos < 0)
    {
      pos = 0;
    }
  } else {
    pos = 0;
  }
  const auto edited = InputString(command, LINES - 2, 23, pos, COLS - 24);
  move(LINES - 2, 1);
  clrtoeol();
  return edited;
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

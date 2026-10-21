#include "core/api.hpp"
#include "core/profile.hpp"
#include "core/ui.hpp"

bool Execute(const DirEntry* dir_entry, const FileEntry* file_entry)
{
  std::string command_line;
  bool result = false;

  if (file_entry && file_entry->meta.IsExecutable())
  {
    command_line = ShellQuote(file_entry->name);
  }

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
  return ui::Prompt("Command:", initial);
}

std::optional<std::string> GetSearchCommandLine()
{
  auto command = GetProfileValueOrEmpty("SEARCHCOMMAND");
  std::size_t pos = 0;

  if (
    const auto placeholder = command.find("{}");
    placeholder != std::string::npos
  )
  {
    pos = placeholder > 0 ? placeholder - 1 : 0;
  }

  return ui::Prompt("Search untag command:", command, 0, pos);
}

bool ExecuteCommand(FileEntry* fe_ptr, const std::string& command)
{
  std::string command_line;

  for (std::size_t i = 0; i < command.length(); ++i)
  {
    const auto c = command[i];

    if (c == '{' && i + 1 < command.length() && command[i + 1] == '}')
    {
      command_line += fe_ptr->GetPath();
      ++i;
    } else {
      command_line.append(1, static_cast<char>(c));
    }
  }

  return SilentSystemCall(command_line);
}

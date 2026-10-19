#include "ui/ui.hpp"

#include <cstdio>
#include <cstdlib>

AppOptions ParseAppOptions(int argc, char** argv)
{
  AppOptions opts;
  for (int argi = 1; argi < argc; ++argi)
  {
    if (*(argv[argi]) != '-')
    {
      opts.initial_dir = argv[argi];
      break;
    }
    switch (*(argv[argi] + 1))
    {
      case 'p':
      case 'P':
        if (*(argv[argi] + 2) <= ' ')
        {
          opts.config_file = argv[++argi];
        }
        else
        {
          opts.config_file = argv[argi] + 2;
        }
        break;

      case 'h':
      case 'H':
        if (*(argv[argi] + 2) <= ' ')
        {
          opts.history_file = argv[++argi];
        }
        else
        {
          opts.history_file = argv[argi] + 2;
        }
        break;

      default:
        std::printf(
          "Usage: %s [-p profile_file] [-h hist_file] [initial_dir]\n",
          argv[0]
        );
        std::exit(EXIT_FAILURE);
        break;
    }
  }
  return opts;
}

std::string ResolveStartupPath(const std::string& path_arg)
{
  std::string startup_path = path_arg;
  if (std::filesystem::path(startup_path).is_relative())
  {
    if (const auto cwd = Getcwd())
    {
      startup_path = (*cwd / path_arg).string();
    }
    else
    {
      startup_path = (std::filesystem::path(".") / path_arg).string();
    }
  }
  return NormPath(startup_path);
}

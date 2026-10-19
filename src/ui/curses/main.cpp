#include "ui/curses/curses_ui.hpp"
#include "ui/ui.hpp"

#include <clocale>
#include <cstdlib>

int main(int argc, char** argv)
{
  std::setlocale(LC_ALL, "");

  const auto opts = ParseAppOptions(argc, argv);
  Init(opts.config_file, opts.history_file);

  const auto path = ResolveStartupPath(opts.initial_dir);

  statistic.login_path.clear();
  statistic.path.clear();

  if (!LoginDisk(path))
  {
    endwin();
    std::exit(1);
  }

  while (true)
  {
    if (HandleDirWindow(statistic.tree.get()) == 'q')
    {
      Quit();
    }
  }
}

#include "ui/curses/curses_ui.hpp"

std::optional<SortSpec> GetKindOfSort()
{
  SortKey key = SortKey::Name;
  SortOrder order = SortOrder::Ascending;
  int c;

  ClearHelp();
  PrintOptions(
    stdscr,
    LINES - 2,
    1,
    "Sort by (A)ccTime (C)hgTime (E)xtension (G)roup (M)odTime   (O)rder: [ascending]"
  );
  PrintOptions(
    stdscr,
    LINES - 1,
    2,
    "       (N)ame o(W)ner (S)ize"
  );

  RefreshWindow(stdscr);
  doupdate();
  do
  {
    c = Getch();

    if (c == -1 || c == ESC)
    {
      return std::nullopt;
    }

    c = std::toupper(c);

    if (c == 'Q')
    {
      return std::nullopt;
    }

    switch (c)
    {
      case 'N':
        key = SortKey::Name;
        break;

      case 'E':
        key = SortKey::Extension;
        break;

      case 'M':
        key = SortKey::ModTime;
        break;

      case 'A':
        key = SortKey::AccTime;
        break;

      case 'C':
        key = SortKey::ChgTime;
        break;

      case 'G':
        key = SortKey::Group;
        break;

      case 'W':
        key = SortKey::Owner;
        break;

      case 'S':
        key = SortKey::Size;
        break;

      case 'O':
        if (order == SortOrder::Ascending)
        {
          PrintOptions(stdscr, LINES - 2, 58, "[descending]");
          order = SortOrder::Descending;
        } else {
          PrintOptions(stdscr, LINES - 2, 58, "[ascending] ");
          order = SortOrder::Ascending;
        }
        RefreshWindow(stdscr);
        doupdate();
        break;

      default:
        beep();
        RefreshWindow(stdscr);
        doupdate();
        break;
    }
  }
  while (!std::strchr("ACEGMNWS", c));

  return std::make_optional<SortSpec>({ key, order });
}

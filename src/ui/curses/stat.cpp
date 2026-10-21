#include "core/profile.hpp"
#include "ui/curses/curses_ui.hpp"
#include "ui/view_info.hpp"

static void PrettyPrintNumber(int y, int x, std::int64_t number);

void PaintDirStatistic(const ui::DirPanelInfo& info)
{
  statistic.path = info.path_line;
  wmove(stdscr, 0, 6);
  wclrtoeol(stdscr);
  Print(
    stdscr,
    0,
    6,
    std::format("{:<{}}", FormFilename(info.path_line, COLS - 10), COLS - 10),
    HIMENUS_COLOR
  );
  PrintOptions(stdscr, 7,  COLS - 24, "[DIR Statistics    ]");
  PrettyPrintNumber(9, COLS - 17, info.total_files);
  PrettyPrintNumber(10, COLS - 17, info.total_bytes);
  PrettyPrintNumber(12, COLS - 17, info.matching_files);
  PrettyPrintNumber(13, COLS - 17, info.matching_bytes);
  PrettyPrintNumber(15, COLS - 17, info.tagged_files);
  PrettyPrintNumber(16, COLS - 17, info.tagged_bytes);
  PrintOptions(stdscr, 17, COLS - 24, "[Current File        ]");
  RefreshWindow(stdscr);
}

void PaintDirTagged(const ui::TaggedCounts& counts)
{
  PrettyPrintNumber(15, COLS - 17, counts.files);
  PrettyPrintNumber(16, COLS - 17, counts.bytes);
  RefreshWindow(stdscr);
}

void PaintDirParameter(const ui::DirPanelInfo& info)
{
  statistic.path = info.path_line;
  wmove(stdscr, 0, 6);
  wclrtoeol(stdscr);
  Print(
    stdscr,
    0,
    6,
    std::format("{:<{}}", FormFilename(info.path_line, COLS - 10), COLS - 10),
    HIMENUS_COLOR
  );
  PrintMenuOptions(
    stdscr,
    18,
    COLS - 22,
    std::format("[{:<20}]", CutFilename(info.name_label, 20)),
    MENU_COLOR,
    HIMENUS_COLOR
  );
  PrettyPrintNumber(19, COLS - 17, info.total_bytes);
  RefreshWindow(stdscr);
}

void PaintGlobalFileParameter(const ui::FilePanelInfo& info)
{
  wmove(stdscr, 0, 6);
  wclrtoeol(stdscr);
  PrintMenuOptions(
    stdscr,
    0,
    6,
    std::format("[{:<{}}]", FormFilename(info.path_line, COLS - 10), COLS - 10),
    GLOBAL_COLOR,
    HIGLOBAL_COLOR
  );
  PrintMenuOptions(
    stdscr,
    18,
    COLS - 22,
    std::format("[{:<20}]", CutFilename(info.name_label, 20)),
    GLOBAL_COLOR,
    HIGLOBAL_COLOR
  );
  PrettyPrintNumber(19, COLS - 17, info.size);
  RefreshWindow(stdscr);
}

void PaintFileParameter(const ui::FilePanelInfo& info)
{
  PrintMenuOptions(
    stdscr,
    18,
    COLS - 22,
    std::format("[{:<20}]", CutFilename(info.name_label, 20)),
    MENU_COLOR,
    HIMENUS_COLOR
  );
  PrettyPrintNumber(19, COLS - 17, info.size);
  RefreshWindow(stdscr);
}

void DisplayDiskStatistic()
{
  PrintMenuOptions(
    stdscr,
    2,
    COLS - 18,
    std::format("[{:<17}]", statistic.file_spec),
    MENU_COLOR,
    HIMENUS_COLOR
  );
  PrettyPrintNumber(5,  COLS - 17, statistic.disk_space / (std::int64_t)1024);
  PrintOptions(stdscr, 7,  COLS - 24, "[DISK Statistics   ]");
  PrettyPrintNumber(9, COLS - 17, statistic.disk_total_files);
  PrettyPrintNumber(10, COLS - 17, statistic.disk_total_bytes);
  PrettyPrintNumber(12, COLS - 17, statistic.disk_matching_files);
  PrettyPrintNumber(13, COLS - 17, statistic.disk_matching_bytes);
  PrettyPrintNumber(15, COLS - 17, statistic.disk_tagged_files);
  PrettyPrintNumber(16, COLS - 17, statistic.disk_tagged_bytes);
  PrintOptions(stdscr, 17, COLS - 24, "[Current Directory    ]");
  DisplayDiskName();
  return;
}



void DisplayAvailBytes()
{
  PrettyPrintNumber(5,  COLS - 17, statistic.disk_space / (std::int64_t)1024);
  RefreshWindow(stdscr);
}




void DisplayFileSpec()
{
  mvwprintw(stdscr, 2,  COLS - 18, "%-17s", statistic.file_spec.c_str());
  RefreshWindow(stdscr);
}


void DisplayDiskName()
{
  PrintMenuOptions(
    stdscr,
    4,
    COLS - 18,
    std::format("[{:<17}]", statistic.disk_name),
    MENU_COLOR,
    HIMENUS_COLOR
  );
  RefreshWindow(stdscr);
}




void DisplayDirStatistic(DirEntry *dir_entry)
{
  PaintDirStatistic(MakeDirPanelInfo(*dir_entry));
}

void DisplayDirTagged(DirEntry *dir_entry)
{
  PaintDirTagged(MakeTaggedCounts(*dir_entry));
}

void DisplayDiskTagged()
{
  PaintDirTagged(ui::TaggedCounts{
    statistic.disk_tagged_files,
    statistic.disk_tagged_bytes
  });
}

void DisplayDirParameter(DirEntry *dir_entry)
{
  PaintDirParameter(MakeDirPanelInfo(*dir_entry));
}

void DisplayGlobalFileParameter(FileEntry *file_entry)
{
  PaintGlobalFileParameter(MakeGlobalFilePanelInfo(*file_entry));
}

void DisplayFileParameter(FileEntry *file_entry)
{
  PaintFileParameter(MakeFilePanelInfo(*file_entry));
}

static void PrettyPrintNumber(int y, int x, std::int64_t number)
{
  const auto number_separator = GetNumberSeparator();
  const auto terra    = (long)   ( number / (std::int64_t) 1000000000000 );
  const auto giga     = (long) ( ( number % (std::int64_t) 1000000000000 ) / (std::int64_t) 1000000000 );
  const auto mega     = (long) ( ( number % (std::int64_t) 1000000000 ) / (std::int64_t) 1000000 );
  const auto kilo     = (long) ( ( number % (std::int64_t) 1000000 ) / (std::int64_t) 1000 );
  const auto one      = (long)   ( number % (std::int64_t) 1000 );

  if (terra)
  {
     /* "123123123123123" */
     PrintMenuOptions(
       stdscr,
       y,
       x,
       std::format("[{:3}{:3}{:03}{:03}{:03}]", terra, giga, mega, kilo, one),
       MENU_COLOR,
       HIMENUS_COLOR
     );
  }
  if (giga)
  {
     /* "123,123,123,123" */
     PrintMenuOptions(
       stdscr,
       y,
       x,
       std::format(
         "[{:3}{}{:03}{}{:03}{}{:03}]",
         giga,
         number_separator,
         mega,
         number_separator,
         kilo,
         number_separator,
         one
       ),
       MENU_COLOR,
       HIMENUS_COLOR
     );
  }
  else if (mega)
  {
     /* "    123,123,123" */
     PrintMenuOptions(
       stdscr,
       y,
       x,
       std::format(
         "[    {:3}{}{:03}{}{:03}]",
         mega,
         number_separator,
         kilo,
         number_separator,
         one
       ),
       MENU_COLOR,
       HIMENUS_COLOR
     );
  }
  else if (kilo)
  {
     /* "        123,123" */
     PrintMenuOptions(
       stdscr,
       y,
       x,
       std::format("[        {:3}{}{:03}]", kilo, number_separator, one),
       MENU_COLOR,
       HIMENUS_COLOR
     );
  } else {
     /* "            123" */
     PrintMenuOptions(
       stdscr,
       y,
       x,
       std::format("[            {:3}]", one),
       MENU_COLOR,
       HIMENUS_COLOR
     );
  }
}

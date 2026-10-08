#include "ytree.h"
#include "patchlev.h"

#include <array>

using HelpPair = std::array<std::string, 2>;

static void PrintMenuLine(WINDOW* win, int y, int x, const std::string& line);
static void PrintLine(WINDOW *win, int y, int x, const std::string& line, int len);
static void DisplayVersion();

static constexpr std::size_t HELP_LABEL_WIDTH = 10;

static const std::vector<std::string> mask =
{
  "5-----------------------2",
  "|FILE:                  |",
  "6-----------------------7",
  "|DISK:                  |",
  "| Avail                 |",
  "6-----------------------7",
  "|DISK Statistics        |",
  "|[Total]                  |",
  "| Files:                |",
  "| Bytes:                |",
  "|[Matching]               |",
  "| Files:                |",
  "| Bytes:                |",
  "|[Tagged]                 |",
  "| Files:                |",
  "| Bytes:                |",
  "|[Current Directory]      |",
  "|                       |",
  "| Bytes:                |"
};


static const char *logo[] =
{
  "                  #                       ",
  "                ##                        ",
  "     ##  ##   #####  ## ###   ####    ####",
  "    ##  ##    ##     ##  ## ##  ##  ##  ##",
  "   ##  ##    ##     ##  ## ######  ###### ",
  "   #####    ## #   ##     ##      ##      ",
  "     ##     ###  ####     ####    ####    ",
  "#####                                     "
};

static const std::string extended_line = "| |                       |";
static const std::string last_line = "3-8-----------------------4";
static const std::string first_line = "1-5";

static const HelpPair disk_dir_help =
{
  "DIR       (A)ttribute (D)elete  (F)ilespec  (G)roup (L)og (M)akedir                 (Q)uit",
  "COMMANDS  (O)wner (R)ename (S)howall (^S)how-tagged (T)ag (U)ntag e(X)ecute   (^F) dirmode"
};

static const HelpPair ll_dir_help =
{
  "DIR       (F)ilespec (L)ogin (S)howall (T)ag (U)ntag e(X)ecute   (^F) dirmode  (Q)uit       ",
  "COMMANDS                                                                    "
};

static const std::string archive_dir_actions =
  "(F)ilespec (L)og (S)howall (T)ag (U)ntag e(X)ecute   (^F) dirmode  (Q)uit         ";
static const std::string archive_dir_commands =
  "COMMANDS                                                                    ";

static const HelpPair disk_file_help =
{
  "FILE      (A)ttribute (C)opy (D)elete (E)dit (F)ilespec (G)roup (H)ex (L)ogin (M)ove    (Q)uit ",
  "COMMANDS  (O)wner (P)ipe (R)ename (S)ort (T)ag (U)ntag (V)iew e(X)ecute pathcop(Y) (^F)ilemode "
};

static const HelpPair ll_file_help =
{
  "FILE      (F)ilespec (L)ogin (S)ort (T)ag (U)ntag e(X)ecute (^F)ilemode      (Q)uit        ",
  "COMMANDS                                                                   "
};

static const std::string archive_file_actions =
  "(C)opy (F)ilespec (H)ex (P)ipe (S)ort (T)ag (U)ntag (V)iew pathcop(Y) (Q)uit         ";
static const std::string archive_file_commands =
  "COMMANDS  (^F)ilemode                                                        ";

/* Overridable USER_MODE banners; start as disk-mode defaults. */
static HelpPair user_dir_help = disk_dir_help;
static HelpPair user_file_help = disk_file_help;

struct HelpContext
{
  const HelpPair& disk;
  HelpPair& user;
  const HelpPair& ll;
  const std::string& archive_actions;
  const std::string& archive_commands;
  const char* archive_suffix;
  const char* profile_key0;
  const char* profile_key1;
};

static const HelpContext dir_help_context =
{
  disk_dir_help,
  user_dir_help,
  ll_dir_help,
  archive_dir_actions,
  archive_dir_commands,
  "-DIR",
  "DIR1",
  "DIR2"
};

static const HelpContext file_help_context =
{
  disk_file_help,
  user_file_help,
  ll_file_help,
  archive_file_actions,
  archive_file_commands,
  "-FILE",
  "FILE1",
  "FILE2"
};

static std::string PadHelpLabel(std::string label)
{
  if (label.size() < HELP_LABEL_WIDTH)
  {
    label.append(HELP_LABEL_WIDTH - label.size(), ' ');
  }

  return label;
}

static HelpPair MakeArchiveHelp(
  std::string_view kind,
  std::string_view suffix,
  const std::string& actions,
  const std::string& commands
)
{
  return {
    PadHelpLabel(std::string(kind) + std::string(suffix)) + actions,
    commands
  };
}

static std::optional<std::string_view> ArchiveHelpKind(Mode m)
{
  switch (m)
  {
    using enum Mode;
    case TAR_FILE_MODE: return "TAR";
    case ZOO_FILE_MODE: return "ZOO";
    case ZIP_FILE_MODE: return "ZIP";
    case LHA_FILE_MODE: return "LHA";
    case ARC_FILE_MODE: return "ARC";
    case RPM_FILE_MODE: return "RPM";
    case RAR_FILE_MODE: return "RAR";
    case TAPE_MODE: return "TAPE";
    case SEVENZIP_FILE_MODE: return "7ZIP";
    default: return std::nullopt;
  }
}

static HelpPair GetHelp(const HelpContext& ctx, Mode m)
{
  if (m == Mode::DISK_MODE)
  {
    return ctx.disk;
  }
  if (m == Mode::USER_MODE)
  {
    return ctx.user;
  }
  if (m == Mode::LL_FILE_MODE)
  {
    return ctx.ll;
  }
  if (const auto kind = ArchiveHelpKind(m))
  {
    return MakeArchiveHelp(
      *kind,
      ctx.archive_suffix,
      ctx.archive_actions,
      ctx.archive_commands
    );
  }
  return ctx.disk;
}

static void DisplayVersion()
{
  const auto version = std::format(
    "ytree Version {}PL{} {} (Werner Bregulla)",
    VERSION,
    PATCHLEVEL,
    VERSIONDATE
  );

  ClearHelp();
  MvAddStr(
    LINES - 2,
    static_cast<unsigned>(COLS - version.length()) >> 1,
    version
  );
}

static void DisplayHelp(const HelpContext& ctx)
{
  if (mode == Mode::USER_MODE)
  {
    if (ctx.user[0] == ctx.disk[0])
    {
      if (const auto cptr = GetProfileValue(ctx.profile_key0))
      {
        ctx.user[0] = *cptr;
      }
    }
    if (ctx.user[1] == ctx.disk[1])
    {
      if (const auto cptr = GetProfileValue(ctx.profile_key1))
      {
        ctx.user[1] = *cptr;
      }
    }
  }

  const auto help = GetHelp(ctx, mode);
  for (std::size_t i = 0; i < help.size(); ++i)
  {
    PrintOptions(stdscr, LINES - 2 + static_cast<int>(i), 0, help[i]);
    clrtoeol();
  }
}

void DisplayDirHelp()
{
  DisplayHelp(dir_help_context);
}

void DisplayFileHelp()
{
  DisplayHelp(file_help_context);
}

void ClearHelp()
{
  for (int i = 0; i < 3; ++i)
  {
    wmove(stdscr, LINES - 3 + i, 0);
    clrtoeol();
  }
}



void DisplayMenu()
{
  int y = 1;
  int    l, c;


  PrintSpecialString(stdscr, 0, 0, "Path: ", MENU_COLOR);
#ifdef COLOR_SUPPORT
  clrtoeol();
#endif

  werase(dir_window);
  werase(big_file_window);
  werase(small_file_window);

  for (const auto& line : mask)
  {
    PrintOptions(stdscr, y, 0, "|");
    PrintOptions(stdscr, y, COLS - 25 , line);
    ++y;
  }
  for (; y < LINES - 4; ++y)
  {
    PrintMenuLine(stdscr, y, 0, extended_line);
  }
  PrintLine(stdscr, DIR_WINDOW_HEIGHT + 2, 0, "6-7", COLS - 25);
  PrintLine(stdscr, 1, 0, first_line, COLS - 25);
  PrintMenuLine(stdscr, y, 0, last_line);

  l = sizeof(logo) / sizeof(logo[0]);
  c = std::strlen(logo[0]);

  for( y=0; y < l; y++ )
  {
    MvWAddStr(dir_window,
         y + ((DIR_WINDOW_HEIGHT - l) >> 1),
         (DIR_WINDOW_WIDTH - c) >> 1,
         logo[y]
    );
  }
  DisplayVersion();

  touchwin(dir_window);
  /* refresh(); */
}

void SwitchToSmallFileWindow()
{
  werase(file_window);
  PrintLine(stdscr, DIR_WINDOW_HEIGHT + 2, 0, "6-7", COLS - 25);
  file_window = small_file_window;
  RefreshWindow(stdscr);
}

void SwitchToBigFileWindow()
{
  werase(file_window);
  RefreshWindow(file_window);
#ifdef COLOR_SUPPORT
  mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X - 1,
        ACS_VLINE | COLOR_PAIR(MENU_COLOR)| A_BOLD);
  mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X + DIR_WINDOW_WIDTH,
           ACS_VLINE | COLOR_PAIR(MENU_COLOR)| A_BOLD);

#else
  mvwaddch(stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
     DIR_WINDOW_X - 1,
     ACS_VLINE
);
  mvwaddch(stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
     DIR_WINDOW_X + DIR_WINDOW_WIDTH,
     ACS_VLINE
);
#endif /* COLOR_SUPPORT */
  file_window = big_file_window;
  RefreshWindow(stdscr);
}


void MapF2Window()
{
  werase(f2_window);
  PrintSpecialString(
    f2_window,
    F2_WINDOW_HEIGHT - 1,
    0,
    std::string(F2_WINDOW_WIDTH, '='),
    HST_COLOR
  );
  RefreshWindow(f2_window);
}


void UnmapF2Window()
{
  werase(f2_window);
  if (file_window == big_file_window)
  {
#ifdef COLOR_SUPPORT
    mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X - 1,
            ACS_VLINE | COLOR_PAIR(MENU_COLOR)| A_BOLD);

    mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X + DIR_WINDOW_WIDTH,
            ACS_VLINE | COLOR_PAIR(MENU_COLOR)| A_BOLD);
#else
    mvwaddch(stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
      DIR_WINDOW_X - 1,
      ACS_VLINE
    );

    mvwaddch(stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
      DIR_WINDOW_X + DIR_WINDOW_WIDTH,
      ACS_VLINE
    );
#endif /* COLOR_SUPPORT */
  }
  touchwin(stdscr);
}

static void PrintMenuLine(WINDOW* win, int y, int x, const std::string& line)
{
  const auto has_paren = line.find('(') != std::string::npos;
  std::size_t p = has_paren ? 2 : 0;
  const std::size_t l = COLS + 2 + (has_paren ? 2 : 0);
  std::size_t i = 1;
  std::string buffer(l, '\0');

  buffer[0] = line[0];
  if (p == 0)
  {
     p = COLS - 25;
  } else {
     p = COLS - 27;
  }
  for (; i < p; i++)
  {
    buffer[i] = line[1];
  }
  std::strncpy(&buffer[i], &line[2], l - i);
  buffer[l - 1] = '\0';
  PrintOptions(stdscr, y, x , buffer);
}

static void PrintLine(
  WINDOW* win,
  int y,
  int x,
  const std::string& line,
  int len
)
{
  std::string buffer;

  if (len <= 0)
  {
    return;
  }
  buffer += line.empty() ? " " : std::string(1, line[0]);
  buffer.append(
    static_cast<std::size_t>(len - 1),
    line.size() > 1 ? line[1] : ' '
  );
  if (line.size() > 2)
  {
    buffer += line.substr(2);
  }
  PrintOptions(stdscr, y, x, buffer);
}

void RefreshWindow(WINDOW* win)
{
  wnoutrefresh(win);
}

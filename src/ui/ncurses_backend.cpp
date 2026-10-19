#include "core/ui.hpp"
#include "ui/mouse.hpp"
#include "ytree.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

void InitClockImpl();
void SuspendClockImpl();

namespace {

class NcursesDialogs final : public ui::IDialogs
{
public:
  void message(const std::string& msg) override
  {
    show_box("E R R O R", msg);
  }

  void warning(const std::string& msg) override
  {
    show_box("W A R N I N G", msg);
  }

  void error(
    const std::string& msg,
    const std::string& module,
    int line
  ) override
  {
    show_box(
      "INTERNAL ERROR",
      msg +
        "*In Module \"" +
        module +
        "\"*Line " +
        std::to_string(line)
    );
  }

  bool confirm(const std::string& message) override
  {
    return choice(message + " (Y/N)", "YN\033") == 'Y';
  }

  int choice(const std::string& msg, const char* term) override
  {
    int c;

    clearHelp();

    curs_set(1);
    leaveok(stdscr, false);
    mvprintw(LINES - 2, 1, "%s", msg.c_str());
    RefreshWindow(stdscr);
    doupdate();
    do
    {
      c = Getch();
#ifdef KEY_MOUSE
      if (c == KEY_MOUSE)
      {
        DecodeMouse(MouseFocus::Overlay);
        continue;
      }
#endif
      if (c >= 0)
      {
        if (std::islower(c))
        {
          c = std::toupper(c);
        }
      }
    } while (c != -1 && !std::strchr(term, c));

    if (c >= 0)
    {
      echochar(c);
    }

    move(LINES - 2, 1);
    clrtoeol();
    leaveok(stdscr, true);
    curs_set(0);

    return c;
  }

  std::optional<std::string> prompt(const ui::PromptRequest& req) override
  {
    clearHelp();

    std::string label(req.label);
    if (!label.empty() && label.back() != ' ')
    {
      label.push_back(' ');
    }

    MvAddStr(LINES - 2, 1, label);
    const int x = 1 + static_cast<int>(label.size());
    const std::size_t max_length = req.max_length != 0
      ? req.max_length
      : static_cast<std::size_t>(std::max(1, COLS - x - 1));

    const auto value = InputString(
      req.initial,
      LINES - 2,
      x,
      req.initial_pos,
      max_length
    );

    move(LINES - 2, 1);
    clrtoeol();
    return value;
  }

  std::optional<ui::AsToPromptResult> promptAsTo(
    const ui::AsToPromptRequest& req
  ) override
  {
    clearHelp();

    MvAddStr(LINES - 3, 1, std::string(req.header));
    MvAddStr(LINES - 2, 1, "AS   ");

    const auto to_file_value = InputString(
      req.as_initial,
      LINES - 2,
      6,
      0,
      static_cast<std::size_t>(std::max(1, COLS - 6))
    );
    if (!to_file_value)
    {
      clearHelp();
      return std::nullopt;
    }

    MvAddStr(LINES - 1, 1, "TO   ");
    const auto to_dir_value = InputString(
      {},
      LINES - 1,
      6,
      0,
      static_cast<std::size_t>(std::max(1, COLS - 6))
    );
    if (!to_dir_value)
    {
      clearHelp();
      return std::nullopt;
    }

    return ui::AsToPromptResult{*to_file_value, *to_dir_value};
  }

  bool editFileMode(std::string& modus, const char* term) override
  {
    static char rwx[] = "rwx";
    const int y = LINES - 2;
    int x = 1;
    int c = 0;
    int p = 0;

    clearHelp();
    curs_set(1);
    MvAddStr(y, x, "New Filemodus:");
    x += 16;

    MvAddStr(y, x, modus);
    leaveok(stdscr, false);
    do
    {
      move(y, x + p);
      RefreshWindow(stdscr);
      doupdate();

      c = Getch();

#ifdef VI_KEYS
      c = ViKey(c);
#endif

      if (c == LF)
      {
        c = CR;
      }

      if (p > 0 && (c == '?' || c == '-' || c == rwx[(p - 1) % 3]))
      {
        modus[static_cast<std::size_t>(p)] = static_cast<char>(c);
        addch(c);
        if (p < 9)
        {
          ++p;
        }
      }
      else if (c == 's' && (p == 3 || p == 6))
      {
        if (modus[static_cast<std::size_t>(p)] != 'x' &&
            modus[static_cast<std::size_t>(p)] != 's')
        {
          message("Execute-Permission required*for set-ID");
        }
        else
        {
          modus[static_cast<std::size_t>(p)] = static_cast<char>(c);
          addch(c);
          if (p < 9)
          {
            ++p;
          }
        }
      }
      else
      {
        if (c == ' ' && p < 9)
        {
          ++p;
        }
        else if (c == KEY_LEFT && p > 0)
        {
          --p;
        }
        else if (c == KEY_RIGHT && p < 9)
        {
          ++p;
        }
        else if (std::strrchr(term, c) == nullptr)
        {
          ::beep();
        }
      }
    } while (c != -1 && std::strrchr(term, c) == nullptr);

    leaveok(stdscr, true);
    move(y, x);
    clrtoeol();
    curs_set(0);

    return c == CR;
  }

  void hitReturnToContinue() override
  {
    std::FILE* tty = std::fopen("/dev/tty", "r+");
    if (tty != nullptr)
    {
      std::fputs("\n[Hit return to continue]", tty);
      std::fflush(tty);
      std::fgetc(tty);
      std::fclose(tty);
      return;
    }

    std::fputs("\n[Hit return to continue]", stdout);
    std::fflush(stdout);
    std::fgetc(stdin);
  }

  void clearHelp() override
  {
    for (int i = 0; i < 3; ++i)
    {
      wmove(stdscr, LINES - 3 + i, 0);
      clrtoeol();
    }
  }

  void clearPromptLines() override
  {
    move(LINES - 3, 1);
    clrtoeol();
    move(LINES - 2, 1);
    clrtoeol();
    move(LINES - 1, 1);
    clrtoeol();
  }

  void refreshDiskStats() override
  {
    DisplayDiskStatistic();
    doupdate();
  }

  void refreshChrome() override
  {
    RefreshWindow(stdscr);
    RefreshWindow(dir_window);
    DisplayMenu();
    doupdate();
  }

  void beep() override
  {
    ::beep();
  }

  bool keyPressed() override
  {
    bool pressed = false;

#if !defined(__linux__)
    nodelay(stdscr, true);
    if (wgetch(stdscr) != ERR)
    {
      pressed = true;
    }
    nodelay(stdscr, false);
#endif

    return pressed;
  }

  bool escapePressed() override
  {
    bool pressed = false;
    int c = 0;

#if !defined(__linux__)
    nodelay(stdscr, true);
    if ((c = wgetch(stdscr)) != ERR)
    {
      pressed = true;
    }
    nodelay(stdscr, false);
#endif

    return pressed && c == ESC;
  }

private:
  static void map_error_window(const std::string& header)
  {
    werase(error_window);
    box(error_window, 0, 0);

    PrintSpecialString(
      error_window,
      ERROR_WINDOW_HEIGHT - 3,
      0,
      "6--------------------------------------7",
      WINERR_COLOR
    );
    wattrset(error_window, A_REVERSE | A_BLINK);
    MvWAddStr(
      error_window,
      ERROR_WINDOW_HEIGHT - 2,
      1,
      "             PRESS ENTER              "
    );
    wattrset(error_window, 0);
    print_error_line(1, header);
  }

  static void unmap_error_window()
  {
    werase(error_window);
    touchwin(stdscr);
    doupdate();
  }

  static void print_error_line(int y, const std::string& str)
  {
    MvWAddStr(
      error_window,
      y,
      (ERROR_WINDOW_WIDTH - static_cast<int>(str.length())) >> 1,
      str
    );
  }

  static void display_error_message(const std::string& msg)
  {
    int count = 0;
    int y = 0;
    int j = 0;
    char buffer[ERROR_WINDOW_WIDTH - 2 + 1];

    for (const auto& c : msg)
    {
      if (c == '*')
      {
        ++count;
      }
    }

    if (count > 3)
    {
      y = 2;
    }
    else if (count > 1)
    {
      y = 3;
    }
    else
    {
      y = 4;
    }

    for (const auto& c : msg)
    {
      if (c == '*')
      {
        buffer[j] = 0;
        print_error_line(y++, buffer);
        j = 0;
      }
      else if (j < static_cast<int>(sizeof(buffer) - 1))
      {
        buffer[j++] = c;
      }
    }
    buffer[j] = 0;
    print_error_line(y, buffer);
  }

  void show_box(const std::string& header, const std::string& msg)
  {
    map_error_window(header);
    display_error_message(msg);
    ::beep();
    RefreshWindow(error_window);
    doupdate();
    wgetch(error_window);
    unmap_error_window();
    touchwin(dir_window);
  }
};

class NcursesSession final : public ui::ISession
{
public:
  void suspend() override
  {
    if (!isendwin())
    {
      endwin();
    }
    reset_shell_mode();
    std::fflush(stdout);
    std::fflush(stderr);
    std::fputc('\n', stdout);
    std::fflush(stdout);
  }

  void resume(bool restart_clock) override
  {
    if (isendwin())
    {
      refresh();
    }
    leaveok(stdscr, true);
    curs_set(0);
    if (restart_clock)
    {
      initClock();
    }
  }

  void shutdown() override
  {
    endwin();
  }

  void initClock() override
  {
    InitClockImpl();
  }

  void suspendClock() override
  {
    SuspendClockImpl();
  }
};

class NcursesInput final : public ui::IInput
{
public:
  ui::InputEvent readEvent() override
  {
    if (resize_request)
    {
      return ui::InputEvent::Special(ui::Key::Resize);
    }

    int c = getch();

#ifdef KEY_RESIZE
    if (c == KEY_RESIZE)
    {
      resize_request = true;
      return ui::InputEvent::Special(ui::Key::Resize);
    }
#endif

#ifdef VI_KEYS
    c = ViKey(c);
#endif

    if (c == LF)
    {
      c = CR;
    }

    return MapRawKey(c);
  }

  int readKey() override
  {
    return ToLegacyRaw(readEvent());
  }

  ui::MouseEvent decodeMouse(ui::MouseFocus focus) override
  {
    return DecodeMouse(focus);
  }

  int translateOverlayMouse(int ch) override
  {
    return TranslateOverlayMouse(ch);
  }

private:
  static ui::InputEvent MapRawKey(int c)
  {
    if (c == KEY_DOWN)
    {
      return ui::InputEvent::Special(ui::Key::Down, c);
    }
    if (c == KEY_UP)
    {
      return ui::InputEvent::Special(ui::Key::Up, c);
    }
    if (c == KEY_LEFT)
    {
      return ui::InputEvent::Special(ui::Key::Left, c);
    }
    if (c == KEY_RIGHT)
    {
      return ui::InputEvent::Special(ui::Key::Right, c);
    }
    if (c == KEY_NPAGE)
    {
      return ui::InputEvent::Special(ui::Key::PageDown, c);
    }
    if (c == KEY_PPAGE)
    {
      return ui::InputEvent::Special(ui::Key::PageUp, c);
    }
    if (c == KEY_HOME)
    {
      return ui::InputEvent::Special(ui::Key::Home, c);
    }
    if (c == KEY_END)
    {
      return ui::InputEvent::Special(ui::Key::End, c);
    }
    if (c == KEY_BTAB)
    {
      return ui::InputEvent::Special(ui::Key::BackTab, c);
    }
    if (c == '\t')
    {
      return ui::InputEvent::Special(ui::Key::Tab, c);
    }
    if (c == ' ')
    {
      return ui::InputEvent::Special(ui::Key::Space, c);
    }
    if (c == CR || c == LF)
    {
      return ui::InputEvent::Special(ui::Key::Enter, CR);
    }
    if (c == ESC)
    {
      return ui::InputEvent::Special(ui::Key::Escape, ESC);
    }
#ifdef KEY_F
    if (c == KEY_F(12))
    {
      return ui::InputEvent{ui::Key::Function, 12, c, {}};
    }
#endif
#ifdef KEY_MOUSE
    if (c == KEY_MOUSE)
    {
      return ui::InputEvent::Special(ui::Key::Mouse, c);
    }
#endif
    if (c == -1)
    {
      return ui::InputEvent::Special(ui::Key::Resize, -1);
    }
    return ui::InputEvent::CharKey(c);
  }

  static int ToLegacyRaw(const ui::InputEvent& ev)
  {
    using ui::Key;
    switch (ev.key)
    {
      case Key::Resize:
        return -1;
      case Key::Mouse:
#ifdef KEY_MOUSE
        return KEY_MOUSE;
#else
        return -1;
#endif
      case Key::Down:
        return KEY_DOWN;
      case Key::Up:
        return KEY_UP;
      case Key::Left:
        return KEY_LEFT;
      case Key::Right:
        return KEY_RIGHT;
      case Key::PageDown:
        return KEY_NPAGE;
      case Key::PageUp:
        return KEY_PPAGE;
      case Key::Home:
        return KEY_HOME;
      case Key::End:
        return KEY_END;
      case Key::Tab:
        return '\t';
      case Key::BackTab:
        return KEY_BTAB;
      case Key::Space:
        return ' ';
      case Key::Enter:
        return CR;
      case Key::Escape:
        return ESC;
      case Key::Function:
#ifdef KEY_F
        if (ev.code == 12)
        {
          return KEY_F(12);
        }
#endif
        return ev.code;
      case Key::Char:
        return ev.code;
      default:
        return -1;
    }
  }
};

class NcursesShellView final : public ui::IShellView
{
public:
  bool refreshDirWindow() override
  {
    return RefreshDirWindow();
  }

  bool internalView(const std::filesystem::path& file_path) override
  {
    return InternalView(file_path);
  }

  void saveHistory() override
  {
    SaveHistory();
  }
};

NcursesDialogs g_dialogs;
NcursesSession g_session;
NcursesInput g_input;
NcursesShellView g_shell;
ui::IDialogs* g_dialogs_ptr = &g_dialogs;
ui::ISession* g_session_ptr = &g_session;
ui::IInput* g_input_ptr = &g_input;
ui::IShellView* g_shell_ptr = &g_shell;

} // namespace

namespace ui {

IDialogs& dialogs()
{
  return *g_dialogs_ptr;
}

ISession& session()
{
  return *g_session_ptr;
}

IInput& input()
{
  return *g_input_ptr;
}

IShellView& shell()
{
  return *g_shell_ptr;
}

void set_backend(
  IDialogs* dialogs_backend,
  ISession* session_backend,
  IInput* input_backend,
  IShellView* shell_backend
)
{
  if (dialogs_backend != nullptr)
  {
    g_dialogs_ptr = dialogs_backend;
  }
  if (session_backend != nullptr)
  {
    g_session_ptr = session_backend;
  }
  if (input_backend != nullptr)
  {
    g_input_ptr = input_backend;
  }
  if (shell_backend != nullptr)
  {
    g_shell_ptr = shell_backend;
  }
}

} // namespace ui

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

/**
 * UI-backend ports. Core talks only to these interfaces so alternate UIs
 * (non-ncurses) can be plugged in later.
 */
namespace ui {

struct PromptRequest
{
  std::string_view label;
  std::string initial;
  /** 0 = let the backend choose a sensible max (e.g. rest of status line). */
  std::size_t max_length = 0;
  std::size_t initial_pos = 0;
};

struct AsToPromptRequest
{
  std::string_view header;
  std::string as_initial;
};

struct AsToPromptResult
{
  std::string as_value;
  std::string to_value;
};

/** Backend-neutral mouse focus for input decoding. */
enum class MouseFocus
{
  Dir,
  File,
  Overlay,
};

enum class MouseAction
{
  None,
  Ignore,
  Select,
  Activate,
  Tag,
  ScrollUp,
  ScrollDown,
  SwitchToDir,
  SwitchToFile,
};

struct MouseEvent
{
  MouseAction action = MouseAction::None;
  int row = 0;
  int col = 0;
};

/** Backend-neutral keys for window command loops. */
enum class Key
{
  None,
  Resize,
  Mouse,
  Up,
  Down,
  Left,
  Right,
  PageUp,
  PageDown,
  Home,
  End,
  Enter,
  Escape,
  Tab,
  BackTab,
  Space,
  /** Printable / control character; see InputEvent::code. */
  Char,
  /** Function key; InputEvent::code is the F-number (e.g. 12). */
  Function,
};

struct InputEvent
{
  Key key = Key::None;
  /** Character code for Key::Char, or Fn number for Key::Function. */
  int code = 0;
  /** Original backend key code (for legacy user-mode remaps). */
  int raw = 0;
  MouseEvent mouse{};

  static InputEvent CharKey(int ch)
  {
    return InputEvent{Key::Char, ch, ch, {}};
  }

  static InputEvent Special(Key key, int raw = 0)
  {
    return InputEvent{key, 0, raw, {}};
  }
};

class IDialogs
{
public:
  virtual ~IDialogs() = default;

  virtual void message(const std::string& msg) = 0;
  virtual void warning(const std::string& msg) = 0;
  virtual void error(
    const std::string& msg,
    const std::string& module,
    int line
  ) = 0;
  virtual bool confirm(const std::string& message) = 0;
  virtual int choice(const std::string& msg, const char* term) = 0;
  virtual std::optional<std::string> prompt(const PromptRequest& req) = 0;
  virtual std::optional<AsToPromptResult> promptAsTo(
    const AsToPromptRequest& req
  ) = 0;
  virtual bool editFileMode(std::string& modus, const char* term) = 0;
  virtual void hitReturnToContinue() = 0;
  virtual void clearHelp() = 0;
  /** Clear the bottom status/prompt lines (not the help strip alone). */
  virtual void clearPromptLines() = 0;
  /** Optional live stats refresh while a long core operation runs. */
  virtual void refreshDiskStats() = 0;
  /** Redraw main chrome (menu / primary windows) after a mode change. */
  virtual void refreshChrome() = 0;
  virtual void beep() = 0;
  virtual bool keyPressed() = 0;
  virtual bool escapePressed() = 0;
};

class ISession
{
public:
  virtual ~ISession() = default;

  /** Leave fullscreen UI so an external program can use the terminal. */
  virtual void suspend() = 0;
  /** Restore fullscreen UI after an external program. */
  virtual void resume(bool restart_clock) = 0;
  /** Tear down the UI (e.g. on quit). */
  virtual void shutdown() = 0;
  virtual void initClock() = 0;
  virtual void suspendClock() = 0;
};

class IInput
{
public:
  virtual ~IInput() = default;

  /** Next normalized input event (resize, mouse, navigation, or character). */
  virtual InputEvent readEvent() = 0;
  /**
   * Legacy raw key code (ncurses-compatible). Prefer readEvent() for new code.
   * Returns -1 on resize.
   */
  virtual int readKey() = 0;
  virtual MouseEvent decodeMouse(MouseFocus focus) = 0;
  /** Map overlay mouse scroll to up/down; other mouse to Key::None raw -1. */
  virtual int translateOverlayMouse(int ch) = 0;
};

/** High-level shell chrome / viewers that core may invoke. */
class IShellView
{
public:
  virtual ~IShellView() = default;

  virtual bool refreshDirWindow() = 0;
  virtual bool internalView(const std::filesystem::path& file_path) = 0;
  virtual void saveHistory() = 0;
};

IDialogs& dialogs();
ISession& session();
IInput& input();
IShellView& shell();

void set_backend(
  IDialogs* dialogs,
  ISession* session,
  IInput* input = nullptr,
  IShellView* shell = nullptr
);

inline void Message(const std::string& msg)
{
  dialogs().message(msg);
}

inline void Warning(const std::string& msg)
{
  dialogs().warning(msg);
}

inline void ErrorEx(
  const std::string& msg,
  const std::string& module,
  int line
)
{
  dialogs().error(msg, module, line);
}

inline bool Confirm(const std::string& message)
{
  return dialogs().confirm(message);
}

inline int Choice(const std::string& msg, const char* term)
{
  return dialogs().choice(msg, term);
}

inline std::optional<std::string> Prompt(
  std::string_view label,
  std::string initial = {},
  std::size_t max_length = 0,
  std::size_t initial_pos = 0
)
{
  return dialogs().prompt(
    PromptRequest{label, std::move(initial), max_length, initial_pos}
  );
}

inline std::optional<AsToPromptResult> PromptAsTo(
  std::string_view header,
  std::string as_initial
)
{
  return dialogs().promptAsTo(
    AsToPromptRequest{header, std::move(as_initial)}
  );
}

inline bool EditFileMode(std::string& modus, const char* term)
{
  return dialogs().editFileMode(modus, term);
}

inline void HitReturnToContinue()
{
  dialogs().hitReturnToContinue();
}

inline void ClearHelp()
{
  dialogs().clearHelp();
}

inline void ClearPromptLines()
{
  dialogs().clearPromptLines();
}

inline void RefreshDiskStats()
{
  dialogs().refreshDiskStats();
}

inline void RefreshChrome()
{
  dialogs().refreshChrome();
}

inline void Beep()
{
  dialogs().beep();
}

inline bool KeyPressed()
{
  return dialogs().keyPressed();
}

inline bool EscapePressed()
{
  return dialogs().escapePressed();
}

inline InputEvent ReadEvent()
{
  return input().readEvent();
}

inline int ReadKey()
{
  return input().readKey();
}

inline bool RefreshDirWindow()
{
  return shell().refreshDirWindow();
}

inline bool InternalView(const std::filesystem::path& file_path)
{
  return shell().internalView(file_path);
}

inline void SaveHistory()
{
  shell().saveHistory();
}

inline void InitClock()
{
  session().initClock();
}

inline void SuspendClock()
{
  session().suspendClock();
}

} // namespace ui

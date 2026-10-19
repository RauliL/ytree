#pragma once

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
};

IDialogs& dialogs();
ISession& session();

void set_backend(IDialogs* dialogs, ISession* session);

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

} // namespace ui

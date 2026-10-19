#include "core/api.hpp"
#include "core/ui.hpp"

void Message(const std::string& msg)
{
  ui::dialogs().message(msg);
}

void Warning(const std::string& msg)
{
  ui::dialogs().warning(msg);
}

void ErrorEx(const std::string& msg, const std::string& module, int line)
{
  ui::dialogs().error(msg, module, line);
}

int InputChoise(const std::string& msg, const char* term)
{
  return ui::dialogs().choice(msg, term);
}

bool Confirm(const std::string& message)
{
  return ui::dialogs().confirm(message);
}

void HitReturnToContinue()
{
  ui::dialogs().hitReturnToContinue();
}

bool KeyPressed()
{
  return ui::dialogs().keyPressed();
}

bool EscapeKeyPressed()
{
  return ui::dialogs().escapePressed();
}

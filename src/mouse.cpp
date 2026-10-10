#include "ytree.h"

#include "./mouse.hpp"

#ifdef NCURSES_MOUSE_VERSION

void EnableMouse()
{
  mmask_t mask =
    BUTTON1_CLICKED |
    BUTTON1_DOUBLE_CLICKED |
    BUTTON3_CLICKED |
#ifdef BUTTON4_PRESSED
    BUTTON4_PRESSED |
#endif
#ifdef BUTTON5_PRESSED
    BUTTON5_PRESSED |
#endif
    0;

  mousemask(mask, nullptr);
  mouseinterval(200);
}

MouseEvent DecodeMouse(MouseFocus focus)
{
  MouseEvent result{};
  MEVENT event;

  result.action = MouseAction::None;

  if (getmouse(&event) != OK)
  {
    return result;
  }

#ifdef BUTTON4_PRESSED
  if (event.bstate & BUTTON4_PRESSED)
  {
    result.action = MouseAction::ScrollUp;
    return result;
  }
#endif
#ifdef BUTTON5_PRESSED
  if (event.bstate & BUTTON5_PRESSED)
  {
    result.action = MouseAction::ScrollDown;
    return result;
  }
#endif

  /* Fullscreen overlays (view, dialogs) only use the scroll wheel. */
  if (focus == MouseFocus::Overlay)
  {
    result.action = MouseAction::Ignore;
    return result;
  }

  const bool left_click =
    (event.bstate & BUTTON1_CLICKED) ||
    (event.bstate & BUTTON1_DOUBLE_CLICKED);
  const bool right_click = (event.bstate & BUTTON3_CLICKED) != 0;
  const bool double_click = (event.bstate & BUTTON1_DOUBLE_CLICKED) != 0;

  if (!left_click && !right_click)
  {
    result.action = MouseAction::Ignore;
    return result;
  }

  int x = event.x;
  int y = event.y;

  if (wenclose(dir_window, y, x))
  {
    if (!wmouse_trafo(dir_window, &y, &x, FALSE))
    {
      result.action = MouseAction::Ignore;
      return result;
    }

    if (focus == MouseFocus::File)
    {
      result.action = MouseAction::SwitchToDir;
      return result;
    }

    result.row = y;
    result.col = 0;
    if (right_click)
    {
      result.action = MouseAction::Tag;
    }
    else if (double_click)
    {
      result.action = MouseAction::Activate;
    }
    else
    {
      result.action = MouseAction::Select;
    }
    return result;
  }

  if (wenclose(file_window, y, x))
  {
    if (!wmouse_trafo(file_window, &y, &x, FALSE))
    {
      result.action = MouseAction::Ignore;
      return result;
    }

    if (focus == MouseFocus::Dir)
    {
      result.action = MouseAction::SwitchToFile;
      return result;
    }

    result.row = y;
    result.col = x;
    if (right_click)
    {
      result.action = MouseAction::Tag;
    }
    else if (double_click)
    {
      result.action = MouseAction::Activate;
    }
    else
    {
      result.action = MouseAction::Select;
    }
    return result;
  }

  result.action = MouseAction::Ignore;
  return result;
}

int TranslateOverlayMouse(int ch)
{
#ifdef KEY_MOUSE
  if (ch != KEY_MOUSE)
  {
    return ch;
  }

  const MouseEvent event = DecodeMouse(MouseFocus::Overlay);

  switch (event.action)
  {
    case MouseAction::ScrollUp:
      return KEY_UP;
    case MouseAction::ScrollDown:
      return KEY_DOWN;
    default:
      return -1;
  }
#else
  return ch;
#endif
}

#else /* !NCURSES_MOUSE_VERSION */

void EnableMouse()
{
}

MouseEvent DecodeMouse(MouseFocus)
{
  MouseEvent result{};
  result.action = MouseAction::None;
  return result;
}

int TranslateOverlayMouse(int ch)
{
  return ch;
}

#endif /* NCURSES_MOUSE_VERSION */

#pragma once

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

void EnableMouse();
MouseEvent DecodeMouse(MouseFocus focus);

/* Maps KEY_MOUSE scroll events to KEY_UP/KEY_DOWN; other mouse input to -1. */
int TranslateOverlayMouse(int ch);

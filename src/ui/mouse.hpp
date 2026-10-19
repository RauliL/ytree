#pragma once

#include "core/ui.hpp"

using ui::MouseFocus;
using ui::MouseAction;
using ui::MouseEvent;

void EnableMouse();
MouseEvent DecodeMouse(MouseFocus focus);

/* Maps KEY_MOUSE scroll events to KEY_UP/KEY_DOWN; other mouse input to -1. */
int TranslateOverlayMouse(int ch);

#pragma once

#include "core/ui.hpp"

ui::IDialogs& GtkDialogsInstance();
ui::ISession& GtkSessionInstance();
ui::IInput& GtkInputInstance();
ui::IShellView& GtkShellViewInstance();
ui::IDirView& GtkDirViewInstance();
ui::IFileView& GtkFileViewInstance();
ui::IViewerSurface& GtkViewerSurfaceInstance();

#include "core/ui.hpp"

#include <stdexcept>
#include <string>

namespace {

ui::IDialogs* g_dialogs_ptr = nullptr;
ui::ISession* g_session_ptr = nullptr;
ui::IInput* g_input_ptr = nullptr;
ui::IShellView* g_shell_ptr = nullptr;
ui::IDirView* g_dir_view_ptr = nullptr;
ui::IFileView* g_file_view_ptr = nullptr;
ui::IViewerSurface* g_viewer_surface_ptr = nullptr;

template <typename T>
T& require(T* ptr, const char* name)
{
  if (ptr == nullptr)
  {
    throw std::logic_error(std::string("UI backend not set: ") + name);
  }
  return *ptr;
}

} // namespace

namespace ui {

IDialogs& dialogs()
{
  return require(g_dialogs_ptr, "IDialogs");
}

ISession& session()
{
  return require(g_session_ptr, "ISession");
}

IInput& input()
{
  return require(g_input_ptr, "IInput");
}

IShellView& shell()
{
  return require(g_shell_ptr, "IShellView");
}

IDirView& dir_view()
{
  return require(g_dir_view_ptr, "IDirView");
}

IFileView& file_view()
{
  return require(g_file_view_ptr, "IFileView");
}

IViewerSurface& viewer_surface()
{
  return require(g_viewer_surface_ptr, "IViewerSurface");
}

void set_backend(
  IDialogs* dialogs_backend,
  ISession* session_backend,
  IInput* input_backend,
  IShellView* shell_backend,
  IDirView* dir_view_backend,
  IFileView* file_view_backend,
  IViewerSurface* viewer_backend
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
  if (dir_view_backend != nullptr)
  {
    g_dir_view_ptr = dir_view_backend;
  }
  if (file_view_backend != nullptr)
  {
    g_file_view_ptr = file_view_backend;
  }
  if (viewer_backend != nullptr)
  {
    g_viewer_surface_ptr = viewer_backend;
  }
}

} // namespace ui

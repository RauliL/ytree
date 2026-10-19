#include <gtkmm.h>

#include "core/ui.hpp"

#ifdef Error
# undef Error
#endif

namespace {

class GtkViewerSurface final : public ui::IViewerSurface
{
public:
  void present() override {}
  void moveCursor(int, int) override {}
  void setCursorVisible(bool) override {}
  void leaveOk(bool) override {}
  void forceFullRedraw() override {}
  void touch() override {}
};

GtkViewerSurface g_viewer;

} // namespace

ui::IViewerSurface& GtkViewerSurfaceInstance()
{
  return g_viewer;
}

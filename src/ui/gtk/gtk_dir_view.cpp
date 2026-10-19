#include <gtkmm.h>

#include "core/ui.hpp"
#include "ui/file_format.hpp"
#include "ui/gtk/main_window.hpp"
#include "ui/ui.hpp"
#include "ui/view_info.hpp"

#include <algorithm>
#include <vector>

#ifdef Error
# undef Error
#endif

namespace {

class GtkDirView final : public ui::IDirView
{
public:
  void getSize(int& height, int& width) override
  {
    height = std::max(8, static_cast<int>(DirListRows().size()));
    width = 80;
    if (auto* win = GtkMainWindow())
    {
      height = std::max(8, win->dirTable().get_height() / 22);
    }
  }

  void showTree(int /*start_entry_no*/, int hilight_no) override
  {
    auto* win = GtkMainWindow();
    if (!win)
    {
      return;
    }

    const ViewMode mode = GetDirViewMode();
    std::vector<TableRow> rows;
    const auto& list = DirListRows();
    rows.reserve(list.size());
    for (const auto& row : list)
    {
      rows.push_back(FormatDirTableRow(row, mode));
    }
    win->setDirTable(mode, rows, hilight_no);
  }

  void showTreeAtCursor() override
  {
    showTree(
      statistic.disp_begin_pos,
      statistic.disp_begin_pos + statistic.cursor_pos
    );
  }

  void showParameter(const ui::DirPanelInfo& info) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setPath(info.path_line);
      win->setCurrentDirectory(info.name_label, info.total_bytes);
    }
  }

  void showDiskStatistic() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->showDiskStatistics();
    }
  }

  void showFileSide() override
  {
    if (DirEntry* dir = CurrentDirListEntry())
    {
      DisplayFileWindow(dir);
    }
  }

  void refresh() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->dirTable().queue_draw();
    }
  }

  void refreshFileSide() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->fileTable().queue_draw();
    }
  }

  void refreshBothFileWindows() override
  {
    refreshFileSide();
  }

  void printEntry(int entry_no, int /*y*/, bool hilight) override
  {
    if (!hilight)
    {
      return;
    }
    if (auto* win = GtkMainWindow())
    {
      win->selectDirRow(entry_no);
    }
  }

  void forceFullRedraw() override
  {
    showTreeAtCursor();
    showFileSide();
  }
};

GtkDirView g_dir_view;

} // namespace

ui::IDirView& GtkDirViewInstance()
{
  return g_dir_view;
}

#include <gtkmm.h>

#include "core/ui.hpp"
#include "ui/gtk/main_window.hpp"
#include "ui/ui.hpp"
#include "ui/view_info.hpp"

#include <algorithm>
#include <string>
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
      height = std::max(8, win->dirList().get_height() / 22);
    }
  }

  void showTree(int start_entry_no, int hilight_no) override
  {
    auto* win = GtkMainWindow();
    if (!win)
    {
      return;
    }

    std::vector<Glib::ustring> rows;
    const auto& list = DirListRows();
    rows.reserve(list.size());
    for (std::size_t i = 0; i < list.size(); ++i)
    {
      const auto& row = list[i];
      std::string indent(row.level * 2, ' ');
      std::string name = row.dir_entry->name.empty() ? "." : row.dir_entry->name;
      if (row.dir_entry->not_scanned)
      {
        name += "/";
      }
      rows.emplace_back(indent + name);
    }
    (void)start_entry_no;
    win->setDirRows(rows, hilight_no);
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
      win->dirList().queue_draw();
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
    showTree(statistic.disp_begin_pos, entry_no);
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

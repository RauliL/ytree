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
    win->dirModel()->splice(0, win->dirModel()->get_n_items(), rows);

    if (auto selection = std::dynamic_pointer_cast<Gtk::SingleSelection>(
          win->dirList().get_model()
        ))
    {
      if (!rows.empty())
      {
        const auto idx = static_cast<guint>(
          std::clamp(hilight_no, 0, static_cast<int>(rows.size()) - 1)
        );
        selection->set_selected(idx);
        (void)start_entry_no;
      }
    }
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
      win->setStatus(
        info.name_label + "  files=" + std::to_string(info.total_files) +
        "  tagged=" + std::to_string(info.tagged_files)
      );
    }
  }

  void showDiskStatistic() override {}

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
      win->fileList().queue_draw();
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

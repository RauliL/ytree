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

void PopulateFileModel(int hilight)
{
  auto* win = GtkMainWindow();
  if (!win)
  {
    return;
  }

  std::vector<Glib::ustring> rows;
  for (const auto& fe : FileListEntries())
  {
    rows.emplace_back(fe->name);
  }
  win->fileModel()->splice(0, win->fileModel()->get_n_items(), rows);

  if (auto selection = std::dynamic_pointer_cast<Gtk::SingleSelection>(
        win->fileList().get_model()
      ))
  {
    if (!rows.empty())
    {
      selection->set_selected(static_cast<guint>(
        std::clamp(hilight, 0, static_cast<int>(rows.size()) - 1)
      ));
    }
  }
}

class GtkFileView final : public ui::IFileView
{
public:
  void getSize(int& height, int& width) override
  {
    height = 24;
    width = 80;
    if (auto* win = GtkMainWindow())
    {
      height = std::max(8, win->fileList().get_height() / 22);
      width = std::max(40, win->fileList().get_width() / 8);
    }
  }

  void getDirPanelSize(int& height, int& width) override
  {
    ui::dir_view().getSize(height, width);
  }

  void showFiles(
    int /*start_file*/,
    int hilight,
    int /*start_x*/,
    bool access_denied
  ) override
  {
    if (FileListEntries().empty())
    {
      auto* win = GtkMainWindow();
      if (!win)
      {
        return;
      }
      win->fileModel()->splice(
        0,
        win->fileModel()->get_n_items(),
        {access_denied ? "Permission Denied!" : "No Files!"}
      );
      return;
    }
    PopulateFileModel(hilight);
  }

  void showFilesAtCursor(
    int start_file,
    int cursor_pos,
    int start_x,
    bool access_denied
  ) override
  {
    showFiles(start_file, start_file + cursor_pos, start_x, access_denied);
  }

  void showFileParameter(const ui::FilePanelInfo& info) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setStatus(
        info.name_label + "  size=" + std::to_string(info.size)
      );
    }
  }

  void showGlobalFileParameter(const ui::FilePanelInfo& info) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setStatus(info.path_line + " / " + info.name_label);
    }
  }

  void showDirStatistic(const ui::DirPanelInfo& info) override
  {
    showFileParameter(ui::FilePanelInfo{info.path_line, info.name_label, info.total_bytes});
  }

  void showDiskStatistic() override {}

  void showDirTagged(const ui::TaggedCounts& counts) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setStatus(
        "Tagged " + std::to_string(counts.files) + " files / " +
        std::to_string(counts.bytes) + " bytes"
      );
    }
  }

  void showDiskTagged() override {}

  void printEntry(
    int entry_no,
    int /*y*/,
    int /*x*/,
    bool hilight,
    int /*start_x*/
  ) override
  {
    if (hilight)
    {
      PopulateFileModel(entry_no);
    }
  }

  void refresh() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->fileList().queue_draw();
    }
  }

  void refreshDirSide() override
  {
    ui::dir_view().refresh();
  }

  void switchToBig() override {}
  void switchToSmall() override {}

  void showDirTree(int start_entry_no, int hilight_no) override
  {
    ui::dir_view().showTree(start_entry_no, hilight_no);
  }

  void forceFullRedraw() override
  {
    refresh();
  }
};

GtkFileView g_file_view;

} // namespace

ui::IFileView& GtkFileViewInstance()
{
  return g_file_view;
}

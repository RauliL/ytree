#include <gtkmm.h>

#include "core/ui.hpp"
#include "ui/file_format.hpp"
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
  rows.reserve(FileListEntries().size());
  for (const auto& fe : FileListEntries())
  {
    rows.emplace_back(FormatFileEntryLine(fe));
  }
  win->setFileRows(rows, hilight);
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
      const int widget_height = win->fileList().get_height();
      const int widget_width = win->fileList().get_width();
      if (widget_height > 0)
      {
        height = std::max(8, widget_height / 22);
      }
      if (widget_width > 0)
      {
        width = std::max(40, widget_width / 8);
      }
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
    auto* win = GtkMainWindow();
    if (!win)
    {
      return;
    }

    if (FileListEntries().empty())
    {
      win->setFileRows(
        {access_denied ? "Permission Denied!" : "No Files!"},
        0
      );
      return;
    }

    const int safe_hilight = hilight < 0 ? 0 : hilight;
    PopulateFileModel(safe_hilight);
  }

  void showFilesAtCursor(
    int start_file,
    int cursor_pos,
    int start_x,
    bool access_denied
  ) override
  {
    const int pos = cursor_pos < 0 ? 0 : cursor_pos;
    showFiles(start_file, start_file + pos, start_x, access_denied);
  }

  void showFileParameter(const ui::FilePanelInfo& info) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setCurrentFile(info.name_label, info.size);
    }
  }

  void showGlobalFileParameter(const ui::FilePanelInfo& info) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setPath(info.path_line);
      win->setCurrentFile(info.name_label, info.size);
    }
  }

  void showDirStatistic(const ui::DirPanelInfo& info) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->showDirStatistics(info);
    }
  }

  void showDiskStatistic() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->showDiskStatistics();
    }
  }

  void showDirTagged(const ui::TaggedCounts& counts) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setTagged(counts);
    }
  }

  void showDiskTagged() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setTagged(ui::TaggedCounts{
        statistic.disk_tagged_files,
        statistic.disk_tagged_bytes
      });
    }
  }

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

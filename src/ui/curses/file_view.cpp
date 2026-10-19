#include "ui/curses/curses_ui.hpp"
#include "ui/file_format.hpp"
#include "ui/ui.hpp"
#include "ui/view_info.hpp"

#include <cstring>

static void PrintFileEntry(int entry_no, int y, int x, unsigned char hilight, int start_x)
{
  static std::string line_buffer;
  static int old_cols = -1;

  char* line_ptr;
  int n;
  const int ef_window_width = GetFileWindowWidth() - 2;
  const int pos_x = x * FileEntryColumnStride();

  if (old_cols != COLS)
  {
    old_cols = COLS;
    line_buffer.resize(COLS + PATH_LENGTH);
  }

  line_buffer = FormatFileEntryLine(FileListEntries()[entry_no]);

  n = StrVisualLength(line_buffer.c_str());

  if (n <= ef_window_width)
  {
    SetFileHideScroll(0, 0);
    line_ptr = line_buffer.data();
  }
  else
  {
    const auto offset = (n > (start_x + ef_window_width))
      ? start_x
      : n - ef_window_width;

    line_ptr = const_cast<char*>(StrVisualIndex(line_buffer.c_str(), offset));
    *const_cast<char*>(StrVisualIndex(line_ptr, ef_window_width)) = '\0';
    SetFileHideScroll(start_x, n - start_x - ef_window_width);
  }

#ifdef NO_HIGHLIGHT
  line_ptr[1] = (hilight) ? '>' : ' ';
  mvwaddstr(file_window, y, pos_x + 1, line_ptr);
#else
#ifdef COLOR_SUPPORT
  if (hilight)
    WbkgdSet(file_window, COLOR_PAIR(HIFILE_COLOR) | A_BOLD);
  else
    WbkgdSet(file_window, COLOR_PAIR(FILE_COLOR));

  mvwaddstr(file_window, y, pos_x + 1, line_ptr);
  WbkgdSet(file_window, COLOR_PAIR(FILE_COLOR) | A_BOLD);

#else
#endif /* COLOR_SUPPORT */
  if (hilight)
    wattrset(file_window, A_REVERSE);
  mvwaddstr(file_window, y, pos_x + 1, line_ptr);
  if (hilight)
    wattrset(file_window, 0);
#endif /* NO_HIGHLIGHT */
}


static void DisplayFiles(
  int start_file_no,
  int hilight_no,
  int start_x,
  bool access_denied
)
{
  int x, y, p_x, p_y, j;

  werase(file_window);

  if (FileListEntries().size() == 0)
  {
    mvwaddstr(
      file_window,
      0,
      3,
      access_denied ? "Permission Denied!" : "No Files!"
    );
  }

  j = start_file_no;
  p_x = -1;
  p_y = 0;
  for (x = 0; x < GetFileMaxColumn(); x++)
  {
    for (y = 0; y < GetFileWindowHeight(); y++)
    {
      if (j < (int)FileListEntries().size())
      {
        if (j == hilight_no)
        {
          p_x = x;
          p_y = y;
        }
        else
        {
          PrintFileEntry(j, y, x, false, start_x);
        }
      }
      j++;
    }
  }

  if (p_x >= 0)
    PrintFileEntry(hilight_no, p_y, p_x, true, start_x);
}



namespace {

class NcursesFileView final : public ui::IFileView
{
public:
  void getSize(int& height, int& width) override
  {
    GetMaxYX(file_window, &height, &width);
  }

  void getDirPanelSize(int& height, int& width) override
  {
    GetMaxYX(dir_window, &height, &width);
  }

  void showFiles(
    int start_file,
    int hilight,
    int start_x,
    bool access_denied
  ) override
  {
    DisplayFiles(start_file, hilight, start_x, access_denied);
  }

  void showFilesAtCursor(
    int start_file,
    int cursor_pos,
    int start_x,
    bool access_denied
  ) override
  {
    DisplayFiles(start_file, start_file + cursor_pos, start_x, access_denied);
  }

  void showFileParameter(const ui::FilePanelInfo& info) override
  {
    PaintFileParameter(info);
  }

  void showGlobalFileParameter(const ui::FilePanelInfo& info) override
  {
    PaintGlobalFileParameter(info);
  }

  void showDirStatistic(const ui::DirPanelInfo& info) override
  {
    PaintDirStatistic(info);
  }

  void showDiskStatistic() override
  {
    DisplayDiskStatistic();
  }

  void showDirTagged(const ui::TaggedCounts& counts) override
  {
    PaintDirTagged(counts);
  }

  void showDiskTagged() override
  {
    DisplayDiskTagged();
  }

  void printEntry(
    int entry_no,
    int y,
    int x,
    bool hilight,
    int start_x
  ) override
  {
    PrintFileEntry(entry_no, y, x, hilight, start_x);
  }

  void refresh() override
  {
    RefreshWindow(file_window);
  }

  void refreshDirSide() override
  {
    RefreshWindow(dir_window);
  }

  void switchToBig() override
  {
    SwitchToBigFileWindow();
  }

  void switchToSmall() override
  {
    SwitchToSmallFileWindow();
  }

  void showDirTree(int start_entry_no, int hilight_no) override
  {
    DisplayTree(dir_window, start_entry_no, hilight_no);
  }

  void forceFullRedraw() override
  {
    clearok(stdscr, true);
  }
};

NcursesFileView g_ncurses_file_view;

} // namespace

ui::IFileView& NcursesFileViewInstance()
{
  return g_ncurses_file_view;
}

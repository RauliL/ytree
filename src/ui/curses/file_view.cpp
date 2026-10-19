#include "ui/curses/curses_ui.hpp"
#include "ui/ui.hpp"
#include "ui/view_info.hpp"

#include <cstring>

static void PrintFileEntry(int entry_no, int y, int x, unsigned char hilight, int start_x)
{
  static std::string line_buffer;
  static int old_cols = -1;

  char justify = GetFileReverseSort() ? '+' : '-';
  char *line_ptr;
  int  n, pos_x = 0;
  std::shared_ptr<FileEntry> fe_ptr;
  int ef_window_width = GetFileWindowWidth() - 2; /* Effektive Window-Width */
  const char* sym_link_name = nullptr;
  char type_of_file = ' ';

  if (old_cols != COLS)
  {
    old_cols = COLS;
    line_buffer.resize(COLS + PATH_LENGTH);
  }

  fe_ptr = FileListEntries()[entry_no];

  if (fe_ptr && fe_ptr->meta.IsLink())
  {
    sym_link_name = fe_ptr->symlink_target.c_str();
  } else {
    sym_link_name = "";
  }

  type_of_file = fe_ptr ? fe_ptr->meta.TypeSymbol() : ' ';

  const auto fitted_name = fe_ptr
    ? FitVisualWidth(fe_ptr->name, GetFileMaxFilenameLen(), justify == '-')
    : std::string{};
  const auto fitted_link = FitVisualWidth(sym_link_name, GetFileMaxLinknameLen(), true);

  switch (GetFileViewMode())
  {
    case ViewMode::MODE_1 : if( fe_ptr )
      {
        const auto attributes = GetAttributes(fe_ptr->meta.Mode());
        const auto modify_time = FormatMetaTime(fe_ptr->meta.mtime);
        const auto nlink = FormatMetaNlink(fe_ptr->meta);

                    if( fe_ptr->meta.IsLink() )
        {
          line_buffer = std::format("{}{}{} {:>10} {:>3} {:11} {:>12} -> {}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              attributes,
              nlink,
                                      (std::int64_t) fe_ptr->meta.size,
              modify_time,
              fitted_link
);
                    }
        else
        {
          line_buffer = std::format("{}{}{} {:>10} {:>3} {:11} {:>12}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              attributes,
              nlink,
                                      (std::int64_t) fe_ptr->meta.size,
              modify_time
);
                    }
      }
      else
      {
        /* Empty Entry */
        /*-------------*/

        line_buffer = std::format("{:<{}}", "", GetFileMaxFilenameLen() + 42);
      }

      if( GetFileMaxLinknameLen() )
        pos_x = x * (GetFileMaxFilenameLen() + GetFileMaxLinknameLen() + 47);
      else
        pos_x = x * (GetFileMaxFilenameLen() + 43);
      break;

    case ViewMode::MODE_2 : if( fe_ptr )
      {
        const auto owner = FormatMetaOwner(fe_ptr->meta);
        const auto group = FormatMetaGroup(fe_ptr->meta);
        const auto inode = FormatMetaInode(fe_ptr->meta);

                    if( fe_ptr->meta.IsLink() )
        {
                      line_buffer = std::format("{}{}{} {:>10} {:<12} {:<12} -> {}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              inode,
              owner,
              group,
              fitted_link
);
                    }
        else
        {
                      line_buffer = std::format("{}{}{} {:>10} {:<12} {:<12}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              inode,
              owner,
              group
);

                    }
            }
      else
      {
        /* Empty-Entry */
        /*-------------*/

        line_buffer = std::format("{:<{}}", "", GetFileMaxFilenameLen() + 38);
      }

      if( GetFileMaxLinknameLen() )
                    pos_x = x * (GetFileMaxFilenameLen() + GetFileMaxLinknameLen() + 43);
      else
                    pos_x = x * (GetFileMaxFilenameLen() + 39);
      break;

    case ViewMode::MODE_3 : if( fe_ptr )
      {
        line_buffer = std::format("{}{}{}",
            (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
            type_of_file,
            fitted_name
);
                  }
      else
      {
        /* Empty-Entry */
        /*-------------*/

        line_buffer = std::format("{:<{}}", "", GetFileMaxFilenameLen() + 2);
      }

      pos_x = x * (GetFileMaxFilenameLen() + 3);
      break;

    case ViewMode::MODE_4 : if( fe_ptr )
      {
        const auto change_time = FormatMetaTime(fe_ptr->meta.ctime);
        const auto access_time = FormatMetaTime(fe_ptr->meta.atime);

                    if( fe_ptr->meta.IsLink() )
        {
                      line_buffer = std::format("{}{}{} Chg: {:>12}  Acc: {:>12} -> {}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              change_time,
              access_time,
              fitted_link
);
                    }
        else
        {
                      line_buffer = std::format("{}{}{} Chg: {:>12}  Acc: {:>12}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              change_time,
              access_time
);
                    }
      }
      else
      {
        /* Empty-Entry */
        /*-------------*/

        line_buffer = std::format("{:<{}}", "", GetFileMaxFilenameLen() + 39);
      }


      if( GetFileMaxLinknameLen() )
        pos_x = x * (GetFileMaxFilenameLen() + GetFileMaxLinknameLen() + 44);
      else
        pos_x = x * (GetFileMaxFilenameLen() + 40);
      break;

    case ViewMode::MODE_5 : if( fe_ptr )
      {
        line_buffer = BuildUserFileEntry(fe_ptr, GetFileMaxFilenameLen(), GetFileMaxLinknameLen(),
            GetProfileValueOrEmpty("USERVIEW"), 200);
      }
      else
      {
        /* Empty-Entry */
        /*-------------*/

        line_buffer = std::format("{:<{}}", "", GetFileMaxUserviewLen());
      }
      pos_x = x * (GetFileMaxUserviewLen() + 1);
      break;

  }

  /* display line */
  /*--------------*/

  n = StrVisualLength(line_buffer.c_str());

  if( n <= ef_window_width )
  {
    /* line fits */
    /*-----------*/

    SetFileHideScroll(0, 0);
    line_ptr = line_buffer.data();
  }
  else
  {
    /* ... does not fit; use start_x */
    /*-------------------------------*/

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
  if( hilight )
    WbkgdSet(file_window, COLOR_PAIR(HIFILE_COLOR)|A_BOLD);
  else
    WbkgdSet(file_window, COLOR_PAIR(FILE_COLOR));

  mvwaddstr(file_window, y, pos_x + 1, line_ptr);
  WbkgdSet(file_window, COLOR_PAIR(FILE_COLOR)|A_BOLD);

#else
#endif /* COLOR_SUPPORT */
  if( hilight ) wattrset(file_window, A_REVERSE);
  mvwaddstr(file_window, y, pos_x + 1, line_ptr);
  if( hilight ) wattrset(file_window, 0);
#endif /* NO_HIGHLIGHT */
}


static void DisplayFiles(
  int start_file_no,
  int hilight_no,
  int start_x,
  bool access_denied
)
{
  int  x, y, p_x, p_y, j;

  werase(file_window);

  if( FileListEntries().size() == 0 )
  {
    mvwaddstr(file_window,
         0,
         3,
         access_denied ? "Permission Denied!" : "No Files!"
);
  }

  j = start_file_no; p_x = -1; p_y = 0;
  for( x=0; x < GetFileMaxColumn(); x++)
  {
    for( y=0; y < GetFileWindowHeight(); y++ )
    {
      if( j < (int)FileListEntries().size() )
      {
  if( j == hilight_no )
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

  if( p_x >= 0 )
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

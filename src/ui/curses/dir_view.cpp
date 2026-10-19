#include "ui/curses/curses_ui.hpp"
#include "ui/ui.hpp"
#include "ui/view_info.hpp"

#include <cstring>
#include <format>

static void PrintDirEntry(WINDOW *win,
                          int entry_no,
                          int y,
                          unsigned char hilight)
{
  unsigned int j, l1, l2;
  int  n;
  int  aux=0;
  int  color, hi_color;
  std::string buffer;
  std::string line_buffer;
  const DirEntry* de_ptr;
  bool suppress_output = false;


  if(win == f2_window) {
    color     = HST_COLOR;
    hi_color  = HIHST_COLOR;
  } else {
    color     = DIR_COLOR;
    hi_color  = HIDIR_COLOR;
  }

  for(j=0; j < DirListRows()[entry_no].level; j++)
  {
    if( DirListRows()[entry_no].indent & ( 1L << j ) )
      buffer += "| ";
    else
      buffer += "  ";
  }
  de_ptr = DirListRows()[entry_no].dir_entry.get();
  if( DirListRows()[entry_no].has_next_sibling )
    buffer += "6-";
  else
    buffer += "3-";

  switch( GetDirViewMode() )
  {
    case ViewMode::MODE_1:
      {
        const auto attributes = GetAttributes(de_ptr->meta.Mode());
        const auto modify_time = FormatMetaTime(de_ptr->meta.mtime);
        line_buffer = std::format(
          "{:>10} {:>3} {:8} {:>12}",
          attributes,
          FormatMetaNlink(de_ptr->meta),
          static_cast<std::int64_t>(de_ptr->meta.size),
          modify_time
);
      }
      break;

    case ViewMode::MODE_2 :
                 {
                   line_buffer = std::format(
                     "{:>12}  {:<12} {:<12}",
                     FormatMetaInode(de_ptr->meta),
                     FormatMetaOwner(de_ptr->meta),
                     FormatMetaGroup(de_ptr->meta)
);
                 }
                 break;
    case ViewMode::MODE_3 :
    case ViewMode::MODE_5 : /* unused for dirs */
      break;
    case ViewMode::MODE_4 :
                 line_buffer = std::format(
                   "Chg.: {:>12}  Acc.: {:>12}",
                   FormatMetaTime(de_ptr->meta.ctime),
                   FormatMetaTime(de_ptr->meta.atime)
);
                 break;
  }

  if(GetDirWindowWidth() == 1)
    suppress_output = true;

  aux = 0;
  /* Output optional Attributes */
  if (!suppress_output) {
     WbkgdSet(win, COLOR_PAIR(color)| A_BOLD);
     if(!line_buffer.empty()) {
       aux = StrVisualLength(line_buffer.c_str());
       if(GetDirWindowWidth() <= aux) {
         TruncateVisual(line_buffer.data(), GetDirWindowWidth() - 1);
         line_buffer.resize(std::strlen(line_buffer.c_str()));
         suppress_output = true;
       }
       mvwaddstr(win, y, 0, line_buffer.c_str());
    }
  }

  if(!suppress_output) {
    /* Output Graph */
    l1 = StrVisualLength(buffer.c_str());
    n = GetDirWindowWidth() - aux;
    if((int)l1 > n) {
       TruncateVisual(buffer.data(), std::max(n - 1, 0));
       buffer.resize(std::strlen(buffer.c_str()));
       suppress_output = true;
    }
    PrintSpecialString(win, y, aux, buffer, color);
  }

  if(!suppress_output) {

    /* Output Dirname */
    buffer = de_ptr->name.empty() ? "." : de_ptr->name;
    if( de_ptr->not_scanned ) {
      buffer += "/";
    }

#ifdef NO_HIGHLIGHT
    buffer += (hilight) ? " <" : "  ";
#else /* NO_HIGHLIGHT */
#ifdef COLOR_SUPPORT
    if( hilight )
      WbkgdSet(win, COLOR_PAIR(hi_color)|A_BOLD);
    else
      WbkgdSet(win, COLOR_PAIR(color));

    n = GetDirWindowWidth() - aux - l1;
    l2 = StrVisualLength(buffer.c_str());
    if((int)l2 > n) {
      TruncateVisual(buffer.data(), std::max(n - 1, 0));
      buffer.resize(std::strlen(buffer.c_str()));
    }

/*    waddstr( win, buffer );*/
    mvwaddstr(win, y, aux + l1, buffer.c_str());
    WbkgdSet(win, COLOR_PAIR(color)|A_BOLD);

#else
    if( hilight ) wattrset(win, A_REVERSE);
/*    waddstr( win, buffer );*/
    mvwaddstr(win, y, aux + l1, buffer.c_str());

    if( hilight ) wattrset(win, 0);
#endif /* COLOR_SUPPORT */
#endif /* NO_HIGHLIGHT */
  }
}




void DisplayTree(WINDOW *win, int start_entry_no, int hilight_no)
{
  int i, y;
  int width, height;

  y = -1;
  GetMaxYX(win, &height, &width);
  if(win == dir_window) {
    SetDirWindowMetrics(height, width);
  }
  for(i=0; i < height; i++)
  {
    wmove(win, i, 0);
    wclrtoeol(win);
  }

  for(i=0; i < height; i++)
  {
    if( start_entry_no + i >= (int)statistic.disk_total_directories ) break;

    if( start_entry_no + i != hilight_no )
      PrintDirEntry(win, start_entry_no + i, i, false);
    else
      y = i;
  }

  if( y >= 0 ) PrintDirEntry(win, start_entry_no + y, y, true);

}


bool KeyF2Get(DirEntry *start_dir_entry,
             int disp_begin_pos,
       int cursor_pos,
             char *path)
{
  int ch;
  bool result = false;
  int win_width, win_height;


  GetMaxYX(f2_window, &win_height, &win_width);
  MapF2Window();
  DisplayTree(f2_window, disp_begin_pos, disp_begin_pos + cursor_pos);
  do
  {
    RefreshWindow(f2_window);
    ui::Present();
    ch = Getch();
    GetMaxYX(f2_window, &win_height, &win_width);  /* Maybe changed... */
    if( ch == LF ) ch = CR;


#ifdef VI_KEYS

    ch = ViKey(ch);

#endif /* VI_KEYS */

    switch( ch )
    {
      case -1:       break;
      case ' ':      break;  /* Quick-Key */
      case KEY_DOWN:
        if (disp_begin_pos + cursor_pos + 1 >= static_cast<int>(DirListRows().size()))
         {
           ui::Beep();
         }
         else
         {
           if( cursor_pos + 1 < win_height )
           {
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, false);
       cursor_pos++;

       PrintDirEntry(f2_window, disp_begin_pos + cursor_pos,
          cursor_pos, true);
                       }
           else
           {
       disp_begin_pos++;
                         DisplayTree(f2_window, disp_begin_pos,
              disp_begin_pos + cursor_pos);
                       }
         }
                     break;

      case KEY_UP  : if( disp_begin_pos + cursor_pos - 1 < 0 )
         {
           ui::Beep();
         }
         else
         {
           if( cursor_pos - 1 >= 0 )
           {
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, false);
       cursor_pos--;
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, true);
                       }
           else
           {
       disp_begin_pos--;
                         DisplayTree(f2_window, disp_begin_pos,
              disp_begin_pos + cursor_pos);
                       }
         }
                     break;

      case KEY_NPAGE:
         if (disp_begin_pos + cursor_pos >= static_cast<int>(DirListRows().size() - 1))
         {
           ui::Beep();
         }
         else
         {
           if( cursor_pos < win_height - 1 )
           {
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, false);
             if (disp_begin_pos + win_height > static_cast<int>(DirListRows().size() - 1))
         cursor_pos = DirListRows().size() - disp_begin_pos - 1;
       else
         cursor_pos = win_height - 1;
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, true);
           }
           else
           {
       if (disp_begin_pos + cursor_pos + win_height < static_cast<int>(DirListRows().size()))
       {
         disp_begin_pos += win_height;
         cursor_pos = win_height - 1;
       }
       else
       {
         disp_begin_pos = DirListRows().size() - win_height;
         if( disp_begin_pos < 0 ) disp_begin_pos = 0;
         cursor_pos = DirListRows().size() - disp_begin_pos - 1;
       }
                         DisplayTree(f2_window, disp_begin_pos,
              disp_begin_pos + cursor_pos);
           }
         }
                     break;

      case KEY_PPAGE:
         if( disp_begin_pos + cursor_pos <= 0 )
         {
           ui::Beep();
         }
         else
         {
           if( cursor_pos > 0 )
           {
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, false);
       cursor_pos = 0;
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, true);
           }
           else
           {
       if( (disp_begin_pos -= win_height) < 0 )
       {
         disp_begin_pos = 0;
       }
                         cursor_pos = 0;
                         DisplayTree(f2_window, disp_begin_pos,
              disp_begin_pos + cursor_pos);
           }
         }
                     break;

      case KEY_HOME: if( disp_begin_pos == 0 && cursor_pos == 0 )
         { ui::Beep(); }
         else
         {
           disp_begin_pos = 0;
           cursor_pos     = 0;
                       DisplayTree(f2_window, disp_begin_pos,
            disp_begin_pos + cursor_pos);
         }
                     break;

      case KEY_END:
        disp_begin_pos = std::max<int>(0, DirListRows().size() - win_height);
         cursor_pos     = DirListRows().size() - disp_begin_pos - 1;
                     DisplayTree(f2_window, disp_begin_pos,
          disp_begin_pos + cursor_pos);
                     break;

      case LF:
      case CR:
      {
        const auto tmp_path = GetPath(DirListRows()[cursor_pos + disp_begin_pos].dir_entry.get());

        std::snprintf(path, PATH_LENGTH + 1, "%s", tmp_path.c_str());
        result = true;
        break;
      }

      case ESC:
      case 'Q':
      case 'q':      break;

      default :      ui::Beep();
         break;
    } /* switch */
  } while( (ch != 'q') && (ch != ESC) && (ch != 'Q') && (ch != CR) && (ch != -1) );

  UnmapF2Window();
  return( result );
}





namespace {

class NcursesDirView final : public ui::IDirView
{
public:
  void getSize(int& height, int& width) override
  {
    GetMaxYX(dir_window, &height, &width);
  }

  void showTree(int start_entry_no, int hilight_no) override
  {
    DisplayTree(dir_window, start_entry_no, hilight_no);
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
    PaintDirParameter(info);
  }

  void showDiskStatistic() override
  {
    DisplayDiskStatistic();
  }

  void showFileSide() override
  {
    if (DirEntry* dir = CurrentDirListEntry())
    {
      DisplayFileWindow(dir);
      RefreshWindow(file_window);
    }
  }

  void refresh() override
  {
    RefreshWindow(dir_window);
  }

  void refreshFileSide() override
  {
    RefreshWindow(file_window);
  }

  void refreshBothFileWindows() override
  {
    RefreshWindow(small_file_window);
    RefreshWindow(big_file_window);
  }

  void printEntry(int entry_no, int y, bool hilight) override
  {
    PrintDirEntry(dir_window, entry_no, y, hilight);
  }

  void forceFullRedraw() override
  {
    clearok(stdscr, true);
  }
};

NcursesDirView g_ncurses_dir_view;

} // namespace

ui::IDirView& NcursesDirViewInstance()
{
  return g_ncurses_dir_view;
}

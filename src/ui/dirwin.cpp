#include <functional>
#include <limits>

#include <peelo/xdg.hpp>

#include "core/commands.hpp"
#include "core/ui.hpp"
#include "ui/mouse.hpp"
#include "ui/view_info.hpp"
#include "ytree.hpp"

struct DirEntryList
{
  std::size_t indent;
  std::shared_ptr<DirEntry> dir_entry;
  unsigned short level;
  bool has_next_sibling;
};

static std::vector<DirEntryList> dir_entry_list;
static std::vector<DirEntryList>::size_type current_dir_entry;
static int window_height;
static int window_width;

static void ReadDirList(const std::vector<std::shared_ptr<DirEntry>>& dir_entries);
static void PrintDirEntry(WINDOW *win, int entry_no, int y, unsigned char hilight);
static void BuildDirEntryList(DirEntry* dir_entry);

static ViewMode dir_mode;

static void BuildDirEntryList(DirEntry* dir_entry)
{
  dir_entry_list.clear();

  /* fuer !ANSI-Systeme.. */
  /*----------------------*/

  /* Skip reserve if the counter has underflowed (unsigned wrap to a huge value). */
  if (statistic.disk_total_directories <= std::numeric_limits<unsigned int>::max() / 2)
  {
    dir_entry_list.reserve(statistic.disk_total_directories);
  }
  current_dir_entry = 0;

  ReadDirList({ SharedDirEntry(dir_entry) });
}

static void RotateDirMode()
{
  switch( dir_mode )
  {
    case ViewMode::MODE_1: dir_mode = ViewMode::MODE_2 ; break;
    case ViewMode::MODE_2: dir_mode = ViewMode::MODE_4 ; break;
    case ViewMode::MODE_3: dir_mode = ViewMode::MODE_1 ; break;
    case ViewMode::MODE_4: dir_mode = ViewMode::MODE_3 ; break;
    case ViewMode::MODE_5: dir_mode = ViewMode::MODE_1 ; break; /* unused for dirs */
  }
  if( (mode != Mode::DISK_MODE && mode != Mode::USER_MODE ) &&
      dir_mode == ViewMode::MODE_4 ) RotateDirMode();
}



static void ReadDirList(const std::vector<std::shared_ptr<DirEntry>>& dir_entries)
{
  static std::size_t indent = 0;
  static int level = 0;

  for (std::size_t i = 0; i < dir_entries.size(); ++i)
  {
    const auto& de_ptr = dir_entries[i];
    const auto has_next_sibling = i + 1 < dir_entries.size();

    indent &= ~(1L << level);
    if (has_next_sibling)
    {
      indent |= ( 1L << level );
    }

    dir_entry_list.push_back({
      indent,
      de_ptr,
      static_cast<unsigned short>(level),
      has_next_sibling,
    });

    ++current_dir_entry;

    if (!de_ptr->not_scanned && !de_ptr->children.empty())
    {
      ++level;
      ReadDirList(de_ptr->children);
      --level;
    }
  }
}

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

  for(j=0; j < dir_entry_list[entry_no].level; j++)
  {
    if( dir_entry_list[entry_no].indent & ( 1L << j ) )
      buffer += "| ";
    else
      buffer += "  ";
  }
  de_ptr = dir_entry_list[entry_no].dir_entry.get();
  if( dir_entry_list[entry_no].has_next_sibling )
    buffer += "6-";
  else
    buffer += "3-";

  switch( dir_mode )
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

  if(window_width == 1)
    suppress_output = true;

  aux = 0;
  /* Output optional Attributes */
  if (!suppress_output) {
     WbkgdSet(win, COLOR_PAIR(color)| A_BOLD);
     if(!line_buffer.empty()) {
       aux = StrVisualLength(line_buffer.c_str());
       if(window_width <= aux) {
         TruncateVisual(line_buffer.data(), window_width - 1);
         line_buffer.resize(std::strlen(line_buffer.c_str()));
         suppress_output = true;
       }
       mvwaddstr(win, y, 0, line_buffer.c_str());
    }
  }

  if(!suppress_output) {
    /* Output Graph */
    l1 = StrVisualLength(buffer.c_str());
    n = window_width - aux;
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

    n = window_width - aux - l1;
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
    window_width  = width;
    window_height = height;
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

static void Movedown(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if (*disp_begin_pos + *cursor_pos + 1 >= static_cast<int>(dir_entry_list.size()))
   {
      /* Element nicht vorhanden */
      /*-------------------------*/
      beep();
   }
   else
   {
      /* Element vorhanden */
      /*-------------------*/
      if( *cursor_pos + 1 < window_height )
      {
          /* Element ist ohne scrollen erreichbar */
          /*--------------------------------------*/
          ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                         *cursor_pos,
                         false
);
          (*cursor_pos)++;
          ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                         *cursor_pos,
                         true
);
      }
      else
      {
          /* Es muss gescrollt werden */
          /*--------------------------*/
          ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                         *cursor_pos,
                         false
);
          scroll(dir_window);
          (*disp_begin_pos)++;
          ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                         *cursor_pos,
                         true
);
      }
      *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry.get();
      (*dir_entry)->start_file = 0;
      (*dir_entry)->cursor_pos = -1;
      ui::dir_view().showFileSide();
   }
   return;
}


static void Moveup(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos - 1 < 0 )
   {
      /* Element nicht vorhanden */
      /*-------------------------*/
      beep();
   }
   else
   {
      /* Element vorhanden */
      /*-------------------*/
      if( *cursor_pos - 1 >= 0 )
      {
         /* Element ist ohne scrollen erreichbar */
         /*--------------------------------------*/
         ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                        *cursor_pos,
                        false
);
         (*cursor_pos)--;
         ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                        *cursor_pos,
                        true
);
      }
      else
      {
          /* Es muss gescrollt werden */
          /*--------------------------*/
          ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                         *cursor_pos,
                         false
);
          wmove(dir_window, 0, 0);
          winsertln(dir_window);
          (*disp_begin_pos)--;
          ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                         *cursor_pos,
                         true
);
      }
      *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry.get();
      (*dir_entry)->start_file = 0;
      (*dir_entry)->cursor_pos = -1;
      ui::dir_view().showFileSide();
   }
   return;
}


static void Movenpage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if (*disp_begin_pos + *cursor_pos >= static_cast<int>(dir_entry_list.size() - 1))
   {
      /* Letzte Position */
      /*-----------------*/
      beep();
   }
   else
   {
      if( *cursor_pos < window_height - 1 )
      {
          /* Cursor steht nicht auf letztem Eintrag
           * der Seite
           */
           ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                          *cursor_pos,
                          false
);
           if( *disp_begin_pos + window_height > static_cast<int>(dir_entry_list.size() - 1))
              *cursor_pos = dir_entry_list.size() - *disp_begin_pos - 1;
           else
              *cursor_pos = window_height - 1;
           *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry.get();
           (*dir_entry)->start_file = 0;
           (*dir_entry)->cursor_pos = -1;
           ui::dir_view().showFileSide();
           ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                          *cursor_pos,
                          true
);
      }
      else
      {
          /* Scrollen */
          /*----------*/
          if( *disp_begin_pos + *cursor_pos + window_height < static_cast<int>(dir_entry_list.size()))
          {
              *disp_begin_pos += window_height;
              *cursor_pos = window_height - 1;
          }
          else
          {
              *disp_begin_pos = dir_entry_list.size() - window_height;
              if( *disp_begin_pos < 0 ) *disp_begin_pos = 0;
              *cursor_pos = dir_entry_list.size() - *disp_begin_pos - 1;
          }
          *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry.get();
          (*dir_entry)->start_file = 0;
          (*dir_entry)->cursor_pos = -1;
          ui::dir_view().showFileSide();
          ui::dir_view().showTree(*disp_begin_pos,*disp_begin_pos+*cursor_pos);
      }
   }
   return;
}

static void Moveppage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos <= 0 )
   {
      /* Erste Position */
      /*----------------*/
      beep();
   }
   else
   {
      if( *cursor_pos > 0 )
      {
          /* Cursor steht nicht auf erstem Eintrag
           * der Seite
           */
           ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                          *cursor_pos,
                          false
);
           *cursor_pos = 0;
           *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry.get();
           (*dir_entry)->start_file = 0;
           (*dir_entry)->cursor_pos = -1;
           ui::dir_view().showFileSide();
           ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos,
                          *cursor_pos,
                          true
);
      }
      else
      {
         /* Scrollen */
         /*----------*/
         if( (*disp_begin_pos -= window_height) < 0 )
         {
             *disp_begin_pos = 0;
         }
         *cursor_pos = 0;
         *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry.get();
         (*dir_entry)->start_file = 0;
         (*dir_entry)->cursor_pos = -1;
         ui::dir_view().showFileSide();
         ui::dir_view().showTree(*disp_begin_pos,*disp_begin_pos+*cursor_pos);
      }
   }
   return;
}

void MoveEnd(DirEntry **dir_entry)
{
    statistic.disp_begin_pos = std::max<int>(0, dir_entry_list.size() - window_height);
    statistic.cursor_pos     = dir_entry_list.size() - statistic.disp_begin_pos - 1;
    *dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry.get();
    (*dir_entry)->start_file = 0;
    (*dir_entry)->cursor_pos = -1;
    ui::dir_view().showFileSide();
    ui::dir_view().showTree(statistic.disp_begin_pos,
    statistic.disp_begin_pos + statistic.cursor_pos);
    return;
}

void MoveHome(DirEntry **dir_entry)
{
    if( statistic.disp_begin_pos == 0 && statistic.cursor_pos == 0 )
    {  /* Position 1 bereits errecht */
       /*----------------------------*/
       beep();
    }
    else
    {
       statistic.disp_begin_pos = 0;
       statistic.cursor_pos     = 0;
       *dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry.get();
       (*dir_entry)->start_file = 0;
       (*dir_entry)->cursor_pos = -1;
       ui::dir_view().showFileSide();
       ui::dir_view().showTree(statistic.disp_begin_pos,
    statistic.disp_begin_pos + statistic.cursor_pos);
    }
    return;
}

static void MoveToRow(int row, int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
  const int index = *disp_begin_pos + row;

  if (row < 0 || row >= window_height ||
      index < 0 || index >= static_cast<int>(dir_entry_list.size()))
  {
    return;
  }

  if (row == *cursor_pos)
  {
    return;
  }

  ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos, *cursor_pos, false);
  *cursor_pos = row;
  ui::dir_view().printEntry(*disp_begin_pos + *cursor_pos, *cursor_pos, true);
  *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry.get();
  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  ui::dir_view().showFileSide();
}

static std::optional<ui::InputEvent> HandleDirMouse(
  int *disp_begin_pos,
  int *cursor_pos,
  DirEntry **dir_entry
)
{
  const MouseEvent event = DecodeMouse(MouseFocus::Dir);

  switch (event.action)
  {
    case MouseAction::ScrollUp:
      return ui::InputEvent::Special(ui::Key::Up);
    case MouseAction::ScrollDown:
      return ui::InputEvent::Special(ui::Key::Down);
    case MouseAction::SwitchToFile:
      return ui::InputEvent::Special(ui::Key::Enter);
    case MouseAction::Select:
      MoveToRow(event.row, disp_begin_pos, cursor_pos, dir_entry);
      return std::nullopt;
    case MouseAction::Activate:
      MoveToRow(event.row, disp_begin_pos, cursor_pos, dir_entry);
      return ui::InputEvent::Special(ui::Key::Enter);
    case MouseAction::Tag:
      MoveToRow(event.row, disp_begin_pos, cursor_pos, dir_entry);
      return ui::InputEvent::CharKey('t');
    case MouseAction::Ignore:
    case MouseAction::None:
    case MouseAction::SwitchToDir:
    default:
      return std::nullopt;
  }
}

static void HandlePlus(
  DirEntry* dir_entry,
  DirEntry* de_ptr,
  char* new_login_path,
  DirEntry* start_dir_entry,
  bool* need_dsp_help
)
{
  if (!dir_entry->not_scanned)
  {
    beep();
    return;
  }
  for (const auto& child : dir_entry->children)
  {
    const auto path = GetPath(child.get());

    std::snprintf(new_login_path, PATH_LENGTH + 1, "%s", path.c_str());
    ReadTree(child, new_login_path, 0);
    child->SetMatchingParam();
  }
  dir_entry->not_scanned = false;
  BuildDirEntryList(start_dir_entry);
  ui::dir_view().showTree(statistic.disp_begin_pos,
    statistic.disp_begin_pos + statistic.cursor_pos
);
  ui::dir_view().showDiskStatistic();
  ui::DisplayAvailBytes();
  *need_dsp_help = true;
}

static void HandleReadSubTree(
  DirEntry* dir_entry,
  DirEntry* start_dir_entry,
  bool& need_dsp_help
)
{
  ScanSubTree(dir_entry);
  BuildDirEntryList(start_dir_entry);
  ui::dir_view().showTree(statistic.disp_begin_pos,
    statistic.disp_begin_pos + statistic.cursor_pos
  );
  ui::dir_view().showDiskStatistic();
  ui::DisplayAvailBytes();
  need_dsp_help = true;
}

static void HandleUnreadSubTree(
  DirEntry* dir_entry,
  DirEntry* de_ptr,
  DirEntry* start_dir_entry,
  bool& need_dsp_help
)
{
  if (dir_entry->not_scanned || dir_entry->children.empty())
  {
    beep();
    return;
  }

  for (const auto& child : dir_entry->children)
  {
    child->UnReadTree();
  }
  dir_entry->not_scanned = true;
  BuildDirEntryList(start_dir_entry);
  ui::dir_view().showTree(statistic.disp_begin_pos,
    statistic.disp_begin_pos + statistic.cursor_pos
  );
  ui::DisplayAvailBytes();
  need_dsp_help = true;
}

static void HandleTagDir(DirEntry* dir_entry, bool value)
{
  for (const auto& fe_sp : dir_entry->files)
  {
    const auto fe_ptr = fe_sp.get();

    if (fe_ptr->matching && fe_ptr->tagged != value)
    {
      fe_ptr->tagged = value;
      if (value)
      {
        ++dir_entry->tagged_files;
        dir_entry->tagged_bytes += fe_ptr->meta.size;
        ++statistic.disk_tagged_files;
        statistic.disk_tagged_bytes += fe_ptr->meta.size;
      } else {
        --dir_entry->tagged_files;
        dir_entry->tagged_bytes -= fe_ptr->meta.size;
        --statistic.disk_tagged_files;
        statistic.disk_tagged_bytes -= fe_ptr->meta.size;
      }
    }
  }

  dir_entry->start_file = 0;
  dir_entry->cursor_pos = -1;
  ui::dir_view().showFileSide();
  ui::dir_view().showDiskStatistic();
}

static void HandleTagAllDirs(DirEntry* dir_entry, bool value)
{
  for (std::size_t i = 0; i < dir_entry_list.size(); ++i)
  {
    for (const auto& fe_sp : dir_entry_list[i].dir_entry->files)
    {
      const auto fe_ptr = fe_sp.get();

      if (fe_ptr->matching && fe_ptr->tagged != value)
      {
        if (value)
        {
          fe_ptr->tagged = value;
          ++dir_entry->tagged_files;
          dir_entry->tagged_bytes += fe_ptr->meta.size;
          ++statistic.disk_tagged_files;
          statistic.disk_tagged_bytes += fe_ptr->meta.size;
        } else {
          fe_ptr->tagged = value;
          --dir_entry->tagged_files;
          dir_entry->tagged_bytes -= fe_ptr->meta.size;
          --statistic.disk_tagged_files;
          statistic.disk_tagged_bytes -= fe_ptr->meta.size;
        }
      }
    }
  }

  dir_entry->start_file = 0;
  dir_entry->cursor_pos = -1;
  ui::dir_view().showFileSide();
  ui::dir_view().showDiskStatistic();
}

static bool HandleShowAllTagged(
  DirEntry* dir_entry,
  DirEntry* start_dir_entry,
  bool& need_dsp_help
)
{
  bool request_change_login = false;
  if (statistic.disk_tagged_files)
  {
    if (dir_entry->login_flag)
    {
      dir_entry->login_flag = false;
    } else {
      dir_entry->big_window  = true;
      dir_entry->global_flag = true;
      dir_entry->tagged_flag = true;
      dir_entry->start_file  = 0;
      dir_entry->cursor_pos  = 0;
    }
    if (HandleFileWindow(dir_entry) != LOGIN_ESC)
    {
      ui::dir_view().showDiskStatistic();
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
      ui::dir_view().showFileSide();
      ui::dir_view().refreshBothFileWindows();
      BuildDirEntryList(start_dir_entry);
      ui::dir_view().showTree(statistic.disp_begin_pos,
        statistic.disp_begin_pos + statistic.cursor_pos
      );
    } else {
      BuildDirEntryList(statistic.tree.get());
      ui::dir_view().showTree(statistic.disp_begin_pos,
      statistic.disp_begin_pos + statistic.cursor_pos);
      request_change_login = true;
    }
  } else {
    dir_entry->login_flag = false;
    beep();
  }
  need_dsp_help = true;
  return request_change_login;
}

static bool HandleShowAll(
  DirEntry* dir_entry,
  DirEntry* start_dir_entry,
  bool& need_dsp_help
)
{
  bool request_change_login = false;
  if (statistic.disk_matching_files)
  {
    if (dir_entry->login_flag)
    {
      dir_entry->login_flag = false;
    } else {
      dir_entry->big_window  = true;
      dir_entry->global_flag = true;
      dir_entry->tagged_flag = false;
      dir_entry->start_file  = 0;
      dir_entry->cursor_pos  = 0;
    }
    if (HandleFileWindow(dir_entry) != LOGIN_ESC)
    {
      ui::dir_view().showDiskStatistic();
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
      ui::dir_view().showFileSide();
      ui::dir_view().refreshBothFileWindows();
      BuildDirEntryList(start_dir_entry);
      ui::dir_view().showTree(statistic.disp_begin_pos,
        statistic.disp_begin_pos + statistic.cursor_pos
      );
    } else {
      BuildDirEntryList(statistic.tree.get());
      ui::dir_view().showTree(statistic.disp_begin_pos,
        statistic.disp_begin_pos + statistic.cursor_pos
      );
      request_change_login = true;
    }
  } else {
    dir_entry->login_flag = false;
    beep();
  }
  need_dsp_help = true;
  return request_change_login;
}

static bool HandleSwitchWindow(
  DirEntry* dir_entry,
  DirEntry* start_dir_entry,
  bool& need_dsp_help
)
{
  bool request_change_login = false;
  if (dir_entry->matching_files)
  {
    if (dir_entry->login_flag)
    {
      dir_entry->login_flag = false;
    } else {
      dir_entry->global_flag = false;
      dir_entry->tagged_flag = false;
      dir_entry->big_window  = bypass_small_window;
      dir_entry->start_file  = 0;
      dir_entry->cursor_pos  = 0;
    }
    if (HandleFileWindow(dir_entry) != LOGIN_ESC)
    {
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
      ui::dir_view().showFileSide();
      ui::dir_view().refreshBothFileWindows();
      BuildDirEntryList(start_dir_entry);
      ui::dir_view().showTree(statistic.disp_begin_pos,
        statistic.disp_begin_pos + statistic.cursor_pos
      );
      ui::dir_view().showDiskStatistic();
    } else {
      BuildDirEntryList(statistic.tree.get());
      ui::dir_view().showTree(statistic.disp_begin_pos,
      statistic.disp_begin_pos + statistic.cursor_pos);
      request_change_login = true;
    }
    ui::DisplayAvailBytes();
    need_dsp_help = true;
  } else {
    dir_entry->login_flag = false;
    beep();
  }
  return request_change_login;
}

struct DirWindowContext
{
  DirEntry* start_dir_entry = nullptr;
  DirEntry* dir_entry = nullptr;
  DirEntry* de_ptr = nullptr;
  std::optional<ui::InputEvent> unput;
  bool need_dsp_help = true;
  char new_login_path[PATH_LENGTH + 1]{};
};

static void DisplayDirTreeAtCursor()
{
  DisplayTree(
    dir_window,
    statistic.disp_begin_pos,
    statistic.disp_begin_pos + statistic.cursor_pos
  );
}

static void RefreshDirEntryFilePane(DirEntry* dir_entry)
{
  dir_entry->start_file = 0;
  dir_entry->cursor_pos = -1;
  ui::dir_view().showFileSide();
}

static void SyncDirEntryFromList(DirEntry*& dir_entry)
{
  dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry.get();
}

static void ApplyStartupDirectorySelection(DirWindowContext& ctx)
{
  if (!initial_directory)
  {
    return;
  }

  std::optional<std::string> home;

  if (!initial_directory->compare("."))
  {
    statistic.disp_begin_pos = 0;
    statistic.cursor_pos = 0;
    ctx.unput = ui::InputEvent::Special(ui::Key::Enter);
  } else {
    if (!initial_directory->empty() && initial_directory->at(1) == '.')
    {
      const auto login = ctx.start_dir_entry->name + initial_directory->substr(1);

      std::snprintf(ctx.new_login_path, PATH_LENGTH + 1, "%s", login.c_str());
    }
    if (
      !initial_directory->empty() &&
      initial_directory->at(1) == '~' &&
      (home = peelo::xdg::home_dir())
    )
    {
      const auto login = *home + initial_directory->substr(1);

      std::snprintf(ctx.new_login_path, PATH_LENGTH + 1, "%s", login.c_str());
    }
    else {
      std::snprintf(
        ctx.new_login_path,
        PATH_LENGTH + 1,
        "%s",
        initial_directory->c_str()
      );
    }

    for (int i = 0; i < static_cast<int>(statistic.disk_total_directories); ++i)
    {
      std::string name;

      if (*ctx.new_login_path == std::filesystem::path::preferred_separator)
      {
        name = GetPath(dir_entry_list[i].dir_entry.get());
      } else {
        name = dir_entry_list[i].dir_entry->name;
      }
      if (name == ctx.new_login_path)
      {
        statistic.disp_begin_pos = i;
        statistic.cursor_pos = 0;
        ctx.unput = ui::InputEvent::Special(ui::Key::Enter);
        break;
      }
    }
  }

  initial_directory.reset();
}

static void InitializeDirWindowDisplay(DirWindowContext& ctx)
{
  ctx.dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry.get();

  ui::dir_view().showDiskStatistic();

  if (!ctx.dir_entry->login_flag)
  {
    ctx.dir_entry->start_file = 0;
    ctx.dir_entry->cursor_pos = -1;
  }

  ui::dir_view().showFileSide();
  ui::dir_view().showTreeAtCursor();
  touchwin(dir_window);

  if (ctx.dir_entry->login_flag)
  {
    if (ctx.dir_entry->global_flag || ctx.dir_entry->tagged_flag)
    {
      ctx.unput = ui::InputEvent::CharKey('S');
    }
    else
    {
      ctx.unput = ui::InputEvent::Special(ui::Key::Enter);
    }
  }
}

static ui::InputEvent ReadDirWindowKey(DirWindowContext& ctx)
{
  if (ctx.unput)
  {
    const auto ev = *ctx.unput;
    ctx.unput.reset();
    return ev;
  }

  doupdate();
  return ui::ReadEvent();
}

static void HandleDirWindowResize(DirEntry* dir_entry, bool& need_dsp_help)
{
  if (!resize_request)
  {
    return;
  }

  ReCreateWindows();
  ui::DisplayMenu();
  ui::dir_view().getSize(window_height, window_width);
  while (statistic.cursor_pos >= window_height)
  {
    statistic.cursor_pos--;
    statistic.disp_begin_pos++;
  }

  ui::dir_view().showTreeAtCursor();
  ui::dir_view().showFileSide();
  ui::dir_view().showDiskStatistic();
  ui::dir_view().showParameter(MakeDirPanelInfo(*dir_entry));
  need_dsp_help = true;
  ui::DisplayAvailBytes();
  ui::DisplayFileSpec();
  ui::DisplayDiskName();
  resize_request = false;
}

static void HandleDirKeyFileSpec(DirEntry* dir_entry, bool& need_dsp_help)
{
  if (ReadFileSpec())
  {
    RefreshDirEntryFilePane(dir_entry);
    ui::dir_view().showDiskStatistic();
  }
  need_dsp_help = true;
}

static void HandleDirKeyMakeDirectory(DirWindowContext& ctx)
{
  if (MakeDirectory(ctx.dir_entry))
  {
    BuildDirEntryList(ctx.start_dir_entry);
    ui::dir_view().showTreeAtCursor();
    ui::DisplayAvailBytes();
  }
  ctx.need_dsp_help = true;
}

static void HandleDirKeyDeleteDirectory(DirWindowContext& ctx)
{
  if (ctx.dir_entry->Delete())
  {
    if (statistic.disp_begin_pos + statistic.cursor_pos > 0)
    {
      if (statistic.cursor_pos > 0)
      {
        statistic.cursor_pos--;
      } else {
        statistic.disp_begin_pos--;
      }
    }
  }

  BuildDirEntryList(ctx.start_dir_entry);
  SyncDirEntryFromList(ctx.dir_entry);
  RefreshDirEntryFilePane(ctx.dir_entry);
  ui::dir_view().showTreeAtCursor();
  ui::DisplayAvailBytes();
  ctx.need_dsp_help = true;
}

static void HandleDirKeyRename(DirWindowContext& ctx)
{
  if (const auto renamed = GetRenameParameter(&ctx.dir_entry->name))
  {
    if (RenameDirectory(ctx.dir_entry, *renamed))
    {
      BuildDirEntryList(ctx.start_dir_entry);
      ui::dir_view().showTreeAtCursor();
      ui::DisplayAvailBytes();
      SyncDirEntryFromList(ctx.dir_entry);
    }
  }
  ctx.need_dsp_help = true;
}

static inline void HandleDirKeyAttributeChange(
  DirEntry* dir_entry,
  bool& need_dsp_help,
  const std::function<bool(DirEntry&)> callback
)
{
  callback(*dir_entry);
  ui::dir_view().showTreeAtCursor();
  need_dsp_help = true;
}

static void HandleDirKeyChangeLogin(DirEntry* dir_entry, bool& need_dsp_help)
{
  std::string login_path;
  if (mode != Mode::DISK_MODE && mode != Mode::USER_MODE)
  {
    login_path = disk_statistic.login_path;
  } else {
    login_path = GetPath(dir_entry);
  }

  if (const auto new_path = GetNewLoginPath(login_path))
  {
    ui::DisplayMenu();
    doupdate();
    LoginDisk(*new_path);
  }
  need_dsp_help = true;
}

static std::optional<int> HandleDirKeyLogParent(DirWindowContext& ctx)
{
  MoveHome(&ctx.dir_entry);

  const auto path = GetPath(ctx.dir_entry);
  std::snprintf(ctx.new_login_path, PATH_LENGTH + 1, "%s", path.c_str());

  char* separator = std::strrchr(
    ctx.new_login_path,
    std::filesystem::path::preferred_separator
  );
  if (separator == nullptr)
  {
    return std::nullopt;
  }

  *separator = '\0';

  if (ctx.new_login_path[0] == '\0')
  {
    ctx.new_login_path[0] = std::filesystem::path::preferred_separator;
    ctx.new_login_path[1] = '\0';
  }

  disk_statistic.login_path.clear();
  LoginDisk(ctx.new_login_path);
  ctx.need_dsp_help = true;
  return WindowExitCode(ui::InputEvent::CharKey('p'));
}

static std::optional<int> ProcessDirWindowKey(ui::InputEvent& ev, DirWindowContext& ctx)
{
  const auto cmd = MapDirCommand(ev);

  switch (cmd)
  {
    case DirCommand::Resize:
      resize_request = true;
      break;

    case DirCommand::Mouse:
    {
      if (const auto injected = HandleDirMouse(
            &statistic.disp_begin_pos,
            &statistic.cursor_pos,
            &ctx.dir_entry
          ))
      {
        ctx.unput = *injected;
      }
      break;
    }

    case DirCommand::None:
      break;

    case DirCommand::MoveDown:
      Movedown(&statistic.disp_begin_pos, &statistic.cursor_pos, &ctx.dir_entry);
      break;

    case DirCommand::MoveUp:
      Moveup(&statistic.disp_begin_pos, &statistic.cursor_pos, &ctx.dir_entry);
      break;

    case DirCommand::PageDown:
      Movenpage(&statistic.disp_begin_pos, &statistic.cursor_pos, &ctx.dir_entry);
      break;

    case DirCommand::PageUp:
      Moveppage(&statistic.disp_begin_pos, &statistic.cursor_pos, &ctx.dir_entry);
      break;

    case DirCommand::Home:
      MoveHome(&ctx.dir_entry);
      break;

    case DirCommand::End:
      MoveEnd(&ctx.dir_entry);
      break;

    case DirCommand::Expand:
      HandlePlus(
        ctx.dir_entry,
        ctx.de_ptr,
        ctx.new_login_path,
        ctx.start_dir_entry,
        &ctx.need_dsp_help
      );
      break;

    case DirCommand::ReadSubtree:
      HandleReadSubTree(
        ctx.dir_entry,
        ctx.start_dir_entry,
        ctx.need_dsp_help
      );
      break;

    case DirCommand::Collapse:
      HandleUnreadSubTree(
        ctx.dir_entry,
        ctx.de_ptr,
        ctx.start_dir_entry,
        ctx.need_dsp_help
      );
      break;

    case DirCommand::FileSpec:
      HandleDirKeyFileSpec(ctx.dir_entry, ctx.need_dsp_help);
      break;

    case DirCommand::Tag:
      HandleTagDir(ctx.dir_entry, true);
      break;

    case DirCommand::Untag:
      HandleTagDir(ctx.dir_entry, false);
      break;

    case DirCommand::TagAll:
      HandleTagAllDirs(ctx.dir_entry, true);
      break;

    case DirCommand::UntagAll:
      HandleTagAllDirs(ctx.dir_entry, false);
      break;

    case DirCommand::RotateMode:
      RotateDirMode();
      ui::dir_view().showTreeAtCursor();
      ui::dir_view().showDiskStatistic();
      ctx.need_dsp_help = true;
      break;

    case DirCommand::ShowTagged:
      if (HandleShowAllTagged(
            ctx.dir_entry,
            ctx.start_dir_entry,
            ctx.need_dsp_help
          ))
      {
        ev = ui::InputEvent::CharKey('L');
      }
      break;

    case DirCommand::ShowAll:
      if (HandleShowAll(
            ctx.dir_entry,
            ctx.start_dir_entry,
            ctx.need_dsp_help
          ))
      {
        ev = ui::InputEvent::CharKey('L');
      }
      break;

    case DirCommand::EnterFiles:
      if (HandleSwitchWindow(
            ctx.dir_entry,
            ctx.start_dir_entry,
            ctx.need_dsp_help
          ))
      {
        ev = ui::InputEvent::CharKey('L');
      }
      break;

    case DirCommand::Execute:
      Execute(ctx.dir_entry, nullptr);
      ctx.need_dsp_help = true;
      ui::DisplayAvailBytes();
      break;

    case DirCommand::MakeDirectory:
      HandleDirKeyMakeDirectory(ctx);
      break;

    case DirCommand::Delete:
      HandleDirKeyDeleteDirectory(ctx);
      break;

    case DirCommand::Rename:
      HandleDirKeyRename(ctx);
      break;

    case DirCommand::ChangeGroup:
      HandleDirKeyAttributeChange(
        ctx.dir_entry,
        ctx.need_dsp_help,
        &DirEntry::ChangeGroup
      );
      break;

    case DirCommand::ChangeOwner:
      HandleDirKeyAttributeChange(
        ctx.dir_entry,
        ctx.need_dsp_help,
        &DirEntry::ChangeOwner
      );
      break;

    case DirCommand::ChangeMode:
      HandleDirKeyAttributeChange(
        ctx.dir_entry,
        ctx.need_dsp_help,
        &DirEntry::ChangeModus
      );
      break;

    case DirCommand::QuitTo:
      ctx.need_dsp_help = true;
      QuitTo(ctx.dir_entry);
      break;

    case DirCommand::Quit:
      ctx.need_dsp_help = true;
      break;

    case DirCommand::ChangeLogin:
      HandleDirKeyChangeLogin(ctx.dir_entry, ctx.need_dsp_help);
      break;

    case DirCommand::Redraw:
      ui::dir_view().forceFullRedraw();
      break;

    case DirCommand::LogParent:
      return HandleDirKeyLogParent(ctx);

    case DirCommand::Unknown:
    default:
      ui::Beep();
      break;
  }

  return std::nullopt;
}

static bool ShouldContinueDirWindow(const ui::InputEvent& ev)
{
  const auto cmd = MapDirCommand(ev);
  return cmd != DirCommand::Quit && cmd != DirCommand::ChangeLogin;
}

int HandleDirWindow(DirEntry *start_dir_entry)
{
  DirWindowContext ctx;
  ctx.start_dir_entry = start_dir_entry;

  ui::dir_view().getSize(window_height, window_width);
  dir_mode = ViewMode::MODE_3;

  BuildDirEntryList(start_dir_entry);
  ApplyStartupDirectorySelection(ctx);
  InitializeDirWindowDisplay(ctx);

  ui::InputEvent ev{};
  do
  {
    if (ctx.need_dsp_help)
    {
      ctx.need_dsp_help = false;
      ui::DisplayDirHelp();
    }

    ui::dir_view().showParameter(MakeDirPanelInfo(*ctx.dir_entry));
    ui::dir_view().refresh();

    ev = ReadDirWindowKey(ctx);
    HandleDirWindowResize(ctx.dir_entry, ctx.need_dsp_help);

    if (mode == Mode::USER_MODE)
    {
      ev = DirUserMode(ctx.dir_entry, ev);
    }

    if (const auto exit_key = ProcessDirWindowKey(ev, ctx))
    {
      return *exit_key;
    }
  } while (ShouldContinueDirWindow(ev));

  return WindowExitCode(ev);
}


void ScanSubTree(DirEntry* dir_entry)
{
  if (dir_entry->not_scanned)
  {
    for (const auto& de_ptr : dir_entry->children)
    {
      ReadTree(de_ptr, de_ptr->GetPath(), 999);
      de_ptr->SetMatchingParam();
    }
    dir_entry->not_scanned = false;
  } else {
    for (const auto& de_ptr : dir_entry->children)
    {
      ScanSubTree(de_ptr.get());
    }
  }
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
    doupdate();
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
        if (disp_begin_pos + cursor_pos + 1 >= static_cast<int>(dir_entry_list.size()))
         {
           beep();
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
           beep();
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
         if (disp_begin_pos + cursor_pos >= static_cast<int>(dir_entry_list.size() - 1))
         {
           beep();
         }
         else
         {
           if( cursor_pos < win_height - 1 )
           {
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, false);
             if (disp_begin_pos + win_height > static_cast<int>(dir_entry_list.size() - 1))
         cursor_pos = dir_entry_list.size() - disp_begin_pos - 1;
       else
         cursor_pos = win_height - 1;
       PrintDirEntry(f2_window,
                      disp_begin_pos + cursor_pos,
          cursor_pos, true);
           }
           else
           {
       if (disp_begin_pos + cursor_pos + win_height < static_cast<int>(dir_entry_list.size()))
       {
         disp_begin_pos += win_height;
         cursor_pos = win_height - 1;
       }
       else
       {
         disp_begin_pos = dir_entry_list.size() - win_height;
         if( disp_begin_pos < 0 ) disp_begin_pos = 0;
         cursor_pos = dir_entry_list.size() - disp_begin_pos - 1;
       }
                         DisplayTree(f2_window, disp_begin_pos,
              disp_begin_pos + cursor_pos);
           }
         }
                     break;

      case KEY_PPAGE:
         if( disp_begin_pos + cursor_pos <= 0 )
         {
           beep();
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
         { beep(); }
         else
         {
           disp_begin_pos = 0;
           cursor_pos     = 0;
                       DisplayTree(f2_window, disp_begin_pos,
            disp_begin_pos + cursor_pos);
         }
                     break;

      case KEY_END:
        disp_begin_pos = std::max<int>(0, dir_entry_list.size() - win_height);
         cursor_pos     = dir_entry_list.size() - disp_begin_pos - 1;
                     DisplayTree(f2_window, disp_begin_pos,
          disp_begin_pos + cursor_pos);
                     break;

      case LF:
      case CR:
      {
        const auto tmp_path = GetPath(dir_entry_list[cursor_pos + disp_begin_pos].dir_entry.get());

        std::snprintf(path, PATH_LENGTH + 1, "%s", tmp_path.c_str());
        result = true;
        break;
      }

      case ESC:
      case 'Q':
      case 'q':      break;

      default :      beep();
         break;
    } /* switch */
  } while( (ch != 'q') && (ch != ESC) && (ch != 'Q') && (ch != CR) && (ch != -1) );

  UnmapF2Window();
  return( result );
}



bool RefreshDirWindow()
{
  DirEntry *de_ptr;
  int i;
  int n;
  bool result = false;
  int window_width, window_height;

  de_ptr = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry.get();
  BuildDirEntryList(dir_entry_list[0].dir_entry.get());

  /* Search old entry */
  for (n = -1, i = 0; i < static_cast<int>(dir_entry_list.size()); ++i)
  {
    if (dir_entry_list[i].dir_entry.get() == de_ptr)
    {
      n = i;
      break;
    }
  }

  if(n == -1) {
    /* Directory disapeared */
    Error("Current directory disappeared");
    result = false;
  } else {

    if( n != (statistic.disp_begin_pos + statistic.cursor_pos)) {
      /* Position changed */
      if((n - statistic.disp_begin_pos) >= 0) {
        statistic.cursor_pos = n - statistic.disp_begin_pos;
      } else {
        statistic.disp_begin_pos = n;
        statistic.cursor_pos = 0;
      }
    }

          ui::dir_view().getSize(window_height, window_width);
          while(statistic.cursor_pos >= window_height) {
      statistic.cursor_pos--;
      statistic.disp_begin_pos++;
          }
    ui::dir_view().showTree(statistic.disp_begin_pos,
          statistic.disp_begin_pos + statistic.cursor_pos
);

    ui::DisplayAvailBytes();
    result = true;
  }

  return(result);
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
    DisplayDirTreeAtCursor();
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
    DirEntry* dir =
      dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos]
        .dir_entry.get();
    DisplayFileWindow(dir);
    RefreshWindow(file_window);
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

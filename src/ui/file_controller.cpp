#include <algorithm>
#include <concepts>
#include <cstring>
#include <functional>
#include <type_traits>

#include "core/commands.hpp"
#include "core/profile.hpp"
#include "core/stat-macros.hpp"
#include "core/ui.hpp"
#include "ui/view_info.hpp"
#include "ui/ui.hpp"

static bool reverse_sort;
static bool order;
static bool do_case = false;
static ViewMode file_mode;
static int  max_column;

static int  window_height;
static int  window_width;
static int  max_disp_files;
static int  x_step;
static int  my_x_step;
static int  hide_left;
static int  hide_right;

void SetFileHideScroll(int left, int right)
{
  hide_left = left;
  hide_right = right;
}

int GetFileHideRight()
{
  return hide_right;
}

/* Snapshot of the files shown in the file window. Shares ownership so that
 * entries removed from the tree stay valid until the list is rebuilt. */
static std::vector<std::shared_ptr<FileEntry>> file_entry_list;
static unsigned      max_userview_len;
static std::size_t max_filename_len;
static std::size_t max_linkname_len;
static std::size_t global_max_filename_len;
static std::size_t global_max_linkname_len;

/* Find the owning shared_ptr of a (non-owning) FileEntry in its directory. */
static std::shared_ptr<FileEntry> FindSharedFileEntry(const FileEntry* fe_ptr)
{
  if (!fe_ptr)
  {
    return nullptr;
  }

  if (const auto dir = fe_ptr->Dir())
  {
    for (const auto& file : dir->files)
    {
      if (file.get() == fe_ptr)
      {
        return file;
      }
    }
  }

  return nullptr;
}

static void ReadFileList(const DirEntry* dir_entry);
static void SortFileEntryList();
static bool SortByName(const FileEntry* e1, const FileEntry* e2);
static bool SortByChgTime(const FileEntry* e1, const FileEntry* e2);
static bool SortByAccTime(const FileEntry* e1, const FileEntry* e2);
static bool SortByModTime(const FileEntry* e1, const FileEntry* e2);
static bool SortBySize(const FileEntry* e1, const FileEntry* e2);
static bool SortByOwner(const FileEntry* e1, const FileEntry* e2);
static bool SortByGroup(const FileEntry* e1, const FileEntry* e2);
static bool SortByExtension(const FileEntry* e1, const FileEntry* e2);
static void ReadGlobalFileList(const DirEntry* dir_entry);
static bool IsMatchingTaggedFiles();
static void RemoveFileEntry(int entry_no);
static void ChangeFileEntry();
static bool DeleteTaggedFiles(int max_dispfiles);
static void RereadWindowSize(DirEntry *dir_entry);
static void ListJump(DirEntry * dir_entry, const char *str);

void SetFileMode(ViewMode new_file_mode)
{
  ui::file_view().getSize(window_height, window_width);
  file_mode = new_file_mode;
  switch( file_mode )
  {
    using enum ViewMode;
    case MODE_1: if( max_linkname_len)
       max_column = window_width /
        (max_filename_len + max_linkname_len + 45);
     else
       max_column = window_width / (max_filename_len + 41);
     break;
    case MODE_2: if( max_linkname_len)
       max_column = window_width /
                   (max_filename_len + max_linkname_len + 41);
     else
                   max_column = window_width / (max_filename_len + 37);
     break;
    case MODE_3: max_column = window_width / (max_filename_len + 3);
         break;
    case MODE_4: if( max_linkname_len)
       max_column = window_width /
        (max_filename_len + max_linkname_len + 44);
     else
       max_column = window_width / (max_filename_len + 40);
     break;
    case MODE_5: max_userview_len = GetUserFileEntryLength(max_filename_len,
                             max_linkname_len,
                             GetProfileValueOrEmpty("USERVIEW"));
                 if(max_userview_len)
       max_column = window_width / (max_userview_len + 1);
     else
       max_column = 0;
     break;
  }

  if( max_column == 0 )
    max_column = 1;
}



void RotateFileMode()
{
  switch( file_mode )
  {
    using enum ViewMode;
    case MODE_1: SetFileMode(MODE_3); break;
    case MODE_2: SetFileMode(MODE_5); break;
    case MODE_3: SetFileMode(MODE_4); break;
    case MODE_4: SetFileMode(MODE_2); break;
    case MODE_5: SetFileMode(MODE_1); break;
  }
  if( (mode != Mode::DISK_MODE && mode != Mode::USER_MODE) && file_mode == ViewMode::MODE_4 ) {
    RotateFileMode();
  } else if(file_mode == ViewMode::MODE_5 && GetProfileValueOrEmpty("USERVIEW").empty()) {
    RotateFileMode();
  }
}

static void ReadTaggedList(const DirEntry* dir_entry)
{
  max_filename_len = 0;
  max_linkname_len = 0;

  for (const auto& fe_ptr : dir_entry->files)
  {
    if (fe_ptr->matching && fe_ptr->tagged)
    {
      const auto name_len = static_cast<std::size_t>(StrVisualLength(fe_ptr->name));

      file_entry_list.push_back(fe_ptr);
      if( fe_ptr->meta.IsLink() )
      {
        const auto linkname_len = static_cast<std::size_t>(
          StrVisualLength(fe_ptr->symlink_target)
        );

        max_linkname_len = std::max(max_linkname_len, linkname_len);
      }
      max_filename_len = std::max(max_filename_len, name_len);
    }
  }
}

static void ReadTaggedFileList(const DirEntry* dir_entry)
{
  for (const auto& child : dir_entry->children)
  {
    ReadTaggedFileList(child.get());
  }
  ReadTaggedList(dir_entry);
  global_max_filename_len = std::max(global_max_filename_len, max_filename_len);
  global_max_linkname_len = std::max(global_max_linkname_len, max_linkname_len);
  max_filename_len = global_max_filename_len;
  max_linkname_len = global_max_linkname_len;
}

static void BuildFileEntryList(DirEntry *dir_entry)
{
  file_entry_list.clear();
  if( !dir_entry->global_flag )  {
     /* ... for !ANSI-Systeme ... */
     /*----------------------------*/
     ReadFileList(dir_entry);
     SortFileEntryList();
     SetFileMode(file_mode); /* recalc */
  }  else if (!dir_entry->tagged_flag)  {
    global_max_filename_len = 0;
    global_max_linkname_len = 0;
    ReadGlobalFileList(statistic.tree.get());
    SortFileEntryList();
    SetFileMode(file_mode); /* recalc */
  } else  {
    global_max_filename_len = 0;
    global_max_linkname_len = 0;
    ReadTaggedFileList(statistic.tree.get());
    SortFileEntryList();
    SetFileMode(file_mode); /* recalc */
  }
}

static void ReadFileList(const DirEntry* dir_entry)
{
  max_filename_len = 0;
  max_linkname_len = 0;

  for (const auto& fe_ptr : dir_entry->files)
  {
    if (fe_ptr->matching)
    {
      const auto name_len = static_cast<std::size_t>(StrVisualLength(fe_ptr->name));

      file_entry_list.push_back(fe_ptr);
      if (fe_ptr->meta.IsLink())
      {
        const auto linkname_len = static_cast<std::size_t>(
          StrVisualLength(fe_ptr->symlink_target)
        );

        max_linkname_len = std::max(max_linkname_len, linkname_len);
      }
      max_filename_len = std::max(max_filename_len, name_len);
    }
  }
}

static void ReadGlobalFileList(const DirEntry* dir_entry)
{
  for (const auto& child : dir_entry->children)
  {
    ReadGlobalFileList(child.get());
  }
  ReadFileList(dir_entry);
  global_max_filename_len = std::max(global_max_filename_len, max_filename_len);
  global_max_linkname_len = std::max(global_max_linkname_len, max_linkname_len);
  max_filename_len = global_max_filename_len;
  max_linkname_len = global_max_linkname_len;
}

static void SortFileEntryList()
{
  std::function<bool(const FileEntry*, const FileEntry*)> compare;

  reverse_sort = false;
  order = statistic.kind_of_sort.order == SortOrder::Ascending;
  switch (statistic.kind_of_sort.key)
  {
    using enum SortKey;
    case Name: compare = SortByName; break;
    case ModTime: compare = SortByModTime; break;
    case ChgTime: compare = SortByChgTime; break;
    case AccTime: compare = SortByAccTime; break;
    case Owner: compare = SortByOwner; break;
    case Group: compare = SortByGroup; break;
    case Size: compare = SortBySize; break;
    case Extension: compare = SortByExtension; break;
  }

  std::sort(
    file_entry_list.begin(),
    file_entry_list.end(),
    [&compare](const std::shared_ptr<FileEntry>& a, const std::shared_ptr<FileEntry>& b)
    {
      return compare(a.get(), b.get());
    }
);
}

static bool SortByName(const FileEntry* e1, const FileEntry* e2)
{
  if (do_case)
     if (order)
        return e1->name < e2->name;
     else
        return e1->name > e2->name;
  else
     if (order)
        return strcasecmp(e1->name.c_str(), e2->name.c_str()) < 0;
     else
        return strcasecmp(e1->name.c_str(), e2->name.c_str()) > 0;
}

static bool SortByExtension(const FileEntry* e1, const FileEntry* e2)
{
  const auto ext1 = e1->GetExtension().value_or("");
  const auto ext2 = e2->GetExtension().value_or("");
  int result;

  /* Ok, this isn't optimized */
  if (
    (do_case && !ext1.compare(ext2)) ||
    (!do_case && !strcasecmp(ext1.c_str(), ext2.c_str()))
  )
  {
    return SortByName(e1, e2);
  }

  if (do_case)
  {
    result = ext1.compare(ext2);
  } else {
    result = strcasecmp(ext1.c_str(), ext2.c_str());
  }

  return (do_case ? result : -result) < 0;
}

static bool SortByModTime(const FileEntry* e1, const FileEntry* e2)
{
  if (order)
     return (e1->meta.MtimeAsTimeT() - e2->meta.MtimeAsTimeT()) < 0;
  else
     return -(e1->meta.MtimeAsTimeT() - e2->meta.MtimeAsTimeT()) < 0;
}

static bool SortByChgTime(const FileEntry* e1, const FileEntry* e2)
{
  if (order)
     return (e1->meta.CtimeAsTimeT() - e2->meta.CtimeAsTimeT()) < 0;
  else
     return -(e1->meta.CtimeAsTimeT() - e2->meta.CtimeAsTimeT()) < 0;
}

static bool SortByAccTime(const FileEntry* e1, const FileEntry* e2)
{
  if (order)
     return (e1->meta.AtimeAsTimeT() - e2->meta.AtimeAsTimeT()) < 0;
  else
     return -(e1->meta.AtimeAsTimeT() - e2->meta.AtimeAsTimeT()) < 0;
}

static bool SortBySize(const FileEntry* e1, const FileEntry* e2)
{
  if (order)
     return (e1->meta.size - e2->meta.size) < 0;
  else
     return -(e1->meta.size - e2->meta.size) < 0;
}

static bool SortByOwner(const FileEntry* e1, const FileEntry* e2)
{
  const auto n1 = FormatMetaOwner(e1->meta);
  const auto n2 = FormatMetaOwner(e2->meta);
  if (do_case)
  {
    if (order)
    {
      return n1.compare(n2) < 0;
    } else {
      return -n1.compare(n2) < 0;
    }
  }
  else if (order)
  {
    return strcasecmp(n1.c_str(), n2.c_str()) < 0;
  } else {
    return -strcasecmp(n1.c_str(), n2.c_str()) < 0;
  }
}

static bool SortByGroup(const FileEntry* e1, const FileEntry* e2)
{
  const auto n1 = FormatMetaGroup(e1->meta);
  const auto n2 = FormatMetaGroup(e2->meta);
  if (do_case)
  {
    if (order)
    {
      return n1.compare(n2) < 0;
    } else {
      return -n1.compare(n2) < 0;
    }
  }
  else if (order)
  {
    return strcasecmp(n1.c_str(), n2.c_str()) < 0;
  } else {
    return -strcasecmp(n1.c_str(), n2.c_str()) < 0;
  }
}

static void RemoveFileEntry(int entry_no)
{
  file_entry_list.erase(file_entry_list.begin() + entry_no);
  ChangeFileEntry();
}

static void ChangeFileEntry()
{

  max_filename_len = 0;
  max_linkname_len = 0;

  for (const auto& entry : file_entry_list)
  {
    const auto length = static_cast<std::size_t>(StrVisualLength(entry->name));

    max_filename_len = std::max(max_filename_len, length);
    if (entry->meta.IsLink())
    {
      max_linkname_len = std::max(
        max_linkname_len,
        static_cast<std::size_t>(StrVisualLength(entry->symlink_target))
);
    }
  }

  SetFileMode(file_mode); // Recalc.
}


void DisplayFileWindow(DirEntry *dir_entry)
{
  ui::file_view().getSize(window_height, window_width);
  BuildFileEntryList(dir_entry);
  ui::file_view().showFiles(
    dir_entry->start_file,
    dir_entry->start_file + dir_entry->cursor_pos,
    0,
    dir_entry->access_denied
  );
}

static void MoveDown(
  int& start_file,
  int& cursor_pos,
  const int start_x,
  DirEntry* dir_entry)
{
  if (start_file + cursor_pos + 1 >= static_cast<int>(file_entry_list.size()))
  {
    // File nicht vorhanden.
    ui::Beep();
    return;
  }

  if (cursor_pos < max_disp_files - 1)
  {
    // DOWN ohne scroll moeglich.
    ui::file_view().printEntry(
      start_file + cursor_pos,
      cursor_pos % window_height,
      cursor_pos / window_height,
      false,
      start_x
    );
    ++cursor_pos;
    ui::file_view().printEntry(
      start_file + cursor_pos,
      cursor_pos % window_height,
      cursor_pos / window_height,
      true ,
      start_x
    );
    return;
  }

  // Scrollen.
  ++start_file;
  ui::file_view().showFiles(start_file, start_file + cursor_pos, start_x, dir_entry->access_denied);
}

static void MoveUp(
  int& start_file,
  int& cursor_pos,
  const int start_x,
  DirEntry* dir_entry
)
{
  if (start_file + cursor_pos < 1)
  {
    // File nicht vorhanden,
    ui::Beep();
    return;
  }

  if (cursor_pos > 0)
  {
    // UP ohne scroll moeglich.
    ui::file_view().printEntry(
      start_file + cursor_pos,
      cursor_pos % window_height,
      cursor_pos / window_height,
      false,
      start_x
    );
    --cursor_pos;
    ui::file_view().printEntry(
      start_file + cursor_pos,
      cursor_pos % window_height,
      cursor_pos / window_height,
      true,
      start_x
    );
    return;
  }

  // Scrollen.
  --start_file;
  ui::file_view().showFiles(start_file, start_file + cursor_pos, start_x, dir_entry->access_denied);
}

static void MoveRight(
  int* start_file,
  int* cursor_pos,
  int* start_x,
  DirEntry* dir_entry
)
{
   if( x_step == 1 )
   {
      /* Sonderfall: ganzes Filewindow scrollen */
      /*----------------------------------------*/
      (*start_x)++;
      ui::file_view().printEntry(*start_file + *cursor_pos,
                      *cursor_pos % window_height,
                      *cursor_pos / window_height,
                      true ,
                      *start_x
);
      if( hide_right < 0 ) (*start_x)--;
   }
   else if( *start_file + *cursor_pos >= (int)file_entry_list.size() - 1 )
   {
      /*letzte Position erreicht */
      /*-------------------------*/
      ui::Beep();
   }
   else
   {
      if( *start_file + *cursor_pos + x_step >= (int)file_entry_list.size() )
      {
          /* voller Step nicht moeglich;
           * auf letzten Eintrag positionieren
           */
           my_x_step = file_entry_list.size() - *start_file - *cursor_pos - 1;
      }
      else
      {
          my_x_step = x_step;
      }
      if( *cursor_pos + my_x_step < max_disp_files )
      {
          /* RIGHT ohne scroll moeglich */
          /*----------------------------*/
          ui::file_view().printEntry(*start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          false,
                          *start_x
);
          *cursor_pos += my_x_step;
          ui::file_view().printEntry(*start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          true ,
                          *start_x
);
      }
      else
      {
          /* Scrollen */
          /*----------*/
          *start_file += x_step;
          *cursor_pos -= x_step - my_x_step;
          ui::file_view().showFiles(*start_file, *start_file + *cursor_pos, *start_x, dir_entry->access_denied);
      }
   }
}


static void MoveLeft(
  int* start_file,
  int* cursor_pos,
  int* start_x,
  DirEntry* dir_entry
)
{
     if( x_step == 1 )
     {
         /* Sonderfall: ganzes Filewindow scrollen */
         /*----------------------------------------*/
         if( *start_x > 0 ) (*start_x)--;
            ui::file_view().printEntry(*start_file + *cursor_pos,
                            *cursor_pos % window_height,
                            *cursor_pos / window_height,
                            true ,
                            *start_x
);
     }
     else if( *start_file + *cursor_pos <= 0 )
     {
         /* erste Position erreicht */
         /*-------------------------*/
         ui::Beep();
     }
     else
     {
         if( *start_file + *cursor_pos - x_step < 0 )
         {
             /* voller Step nicht moeglich;
              * auf ersten Eintrag positionieren
              */
              my_x_step = *start_file + *cursor_pos;
         }
         else
         {
             my_x_step = x_step;
         }
         if( *cursor_pos - my_x_step >= 0 )
         {
             /* LEFT ohne scroll moeglich */
             /*---------------------------*/
             ui::file_view().printEntry(*start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             false,
                             *start_x
);
             *cursor_pos -= my_x_step;
             ui::file_view().printEntry(*start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             true,
                             *start_x
);
         }
         else
         {
             /* Scrollen */
             /*----------*/
             if( ( *start_file -= x_step ) < 0 )
                *start_file = 0;
             ui::file_view().showFiles(*start_file, *start_file + *cursor_pos, *start_x, dir_entry->access_denied);
         }
     }
     return;
}


static void fmovenpage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   if( *start_file + *cursor_pos >= (int)file_entry_list.size() - 1 )
   {
      /*letzte Position erreicht */
      /*-------------------------*/
      ui::Beep();
   }
   else
   {
      if( *cursor_pos < max_disp_files - 1 )
      {
        /* Cursor steht noch nicht auf letztem
         * Eintrag
         * ==> setzen
         */
         ui::file_view().printEntry(*start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         false,
                         *start_x
);
         if( *start_file + max_disp_files <= (int)file_entry_list.size() - 1 )
            *cursor_pos = max_disp_files - 1;
         else
            *cursor_pos = file_entry_list.size() - *start_file - 1;
         ui::file_view().printEntry(*start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         true,
                         *start_x
);
      }
      else
      {
        /* Scrollen */
        /*----------*/
        if( *start_file + *cursor_pos + max_disp_files < (int)file_entry_list.size() )
           *start_file += max_disp_files;
        else
           *start_file = file_entry_list.size() - max_disp_files;
        if( *start_file + max_disp_files <= (int)file_entry_list.size() - 1 )
           *cursor_pos = max_disp_files - 1;
        else
           *cursor_pos = file_entry_list.size() - *start_file - 1;
        ui::file_view().showFiles(*start_file, *start_file + *cursor_pos, *start_x, dir_entry->access_denied);
      }
   }
   return;
}



static void fmoveppage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
     if( *start_file + *cursor_pos <= 0 )
     {
        /* erste Position erreicht */
        /*-------------------------*/
        ui::Beep();
     }
     else
     {
        if( *cursor_pos > 0 )
        {
            /* Cursor steht noch nicht auf erstem
             * Eintrag
             * ==> setzen
             */
             ui::file_view().printEntry(*start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             false,
                             *start_x
);
             *cursor_pos = 0;
             ui::file_view().printEntry(*start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             true,
                             *start_x
);
        }
        else
        {
            /* Scrollen */
            /*----------*/
            if( *start_file > max_disp_files )
               *start_file -= max_disp_files;
            else
               *start_file = 0;
            ui::file_view().showFiles(*start_file, *start_file + *cursor_pos, *start_x, dir_entry->access_denied);
        }
     }
     return;
}

static void fmoveto(int target_pos, int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
  if (target_pos < 0 || target_pos >= static_cast<int>(file_entry_list.size()))
  {
    return;
  }

  const int absolute = *start_file + *cursor_pos;
  if (target_pos == absolute)
  {
    return;
  }

  if (target_pos >= *start_file && target_pos < *start_file + max_disp_files)
  {
    ui::file_view().printEntry(*start_file + *cursor_pos,
                   *cursor_pos % window_height,
                   *cursor_pos / window_height,
                   false,
                   *start_x);
    *cursor_pos = target_pos - *start_file;
    ui::file_view().printEntry(*start_file + *cursor_pos,
                   *cursor_pos % window_height,
                   *cursor_pos / window_height,
                   true,
                   *start_x);
  }
  else
  {
    *start_file = target_pos;
    *cursor_pos = 0;
    if (*start_file + max_disp_files > static_cast<int>(file_entry_list.size()))
    {
      *start_file = std::max(0, static_cast<int>(file_entry_list.size()) - max_disp_files);
      *cursor_pos = target_pos - *start_file;
    }
    ui::file_view().showFiles(*start_file, *start_file + *cursor_pos, *start_x, dir_entry->access_denied);
  }
}

static int FileIndexFromMouse(int row, int col)
{
  if (row < 0 || row >= window_height || col < 0)
  {
    return -1;
  }

  const int column_width = std::max(1, window_width / max_column);
  const int column = col / column_width;

  if (column < 0 || column >= max_column)
  {
    return -1;
  }

  return column * window_height + row;
}

static std::optional<ui::InputEvent> HandleFileMouse(
  int *start_file,
  int *cursor_pos,
  int *start_x,
  DirEntry *dir_entry
)
{
  const ui::MouseEvent event = ui::input().decodeMouse(ui::MouseFocus::File);

  switch (event.action)
  {
    case ui::MouseAction::ScrollUp:
      return ui::InputEvent::Special(ui::Key::Up);
    case ui::MouseAction::ScrollDown:
      return ui::InputEvent::Special(ui::Key::Down);
    case ui::MouseAction::SwitchToDir:
      return ui::InputEvent::Special(ui::Key::Escape);
    case ui::MouseAction::Select:
    case ui::MouseAction::Activate:
    case ui::MouseAction::Tag:
    {
      int target = -1;
      if (event.index >= 0)
      {
        target = event.index;
      }
      else
      {
        const int relative = FileIndexFromMouse(event.row, event.col);
        if (relative < 0)
        {
          return std::nullopt;
        }
        target = *start_file + relative;
      }
      fmoveto(target, start_file, cursor_pos, start_x, dir_entry);
      if (event.action == ui::MouseAction::Activate)
      {
        return ui::InputEvent::CharKey('v');
      }
      if (event.action == ui::MouseAction::Tag)
      {
        return ui::InputEvent::CharKey('t');
      }
      return std::nullopt;
    }
    case ui::MouseAction::Ignore:
    case ui::MouseAction::None:
    case ui::MouseAction::SwitchToFile:
    default:
      return std::nullopt;
  }
}

/* Result of one interactive tagged-file action.
 * replacement non-null means the list slot should track a different entry
 * (e.g. after a move into another directory). */
struct TaggedFileWalkResult
{
  bool continue_walk = true;
  FileEntry* replacement = nullptr;
};

template<typename F>
static TaggedFileWalkResult InvokeTaggedFileAction(F&& action, FileEntry& fe)
{
  using R = std::decay_t<std::invoke_result_t<F&, FileEntry&>>;

  if constexpr (std::is_same_v<R, TaggedFileWalkResult>)
  {
    return action(fe);
  }
  else if constexpr (std::is_same_v<R, bool>)
  {
    return TaggedFileWalkResult{action(fe), nullptr};
  }
  else
  {
    static_assert(
      std::is_void_v<R>,
      "tagged-file walk callback must return void, bool, or TaggedFileWalkResult"
    );
    action(fe);
    return {};
  }
}

template<typename F>
  requires std::invocable<F&, FileEntry&>
static void WalkTaggedFiles(int start_file, int cursor_pos, F&& action)
{
  int start_x = 0;
  bool continue_walk = true;
  bool maybe_change_x = false;

  max_disp_files = window_height * max_column;

  for (
    int i = 0;
    i < static_cast<int>(file_entry_list.size()) && continue_walk;
    ++i
  )
  {
    auto fe_ptr = file_entry_list[i].get();

    if (!fe_ptr->tagged || !fe_ptr->matching)
    {
      continue;
    }

    if (
      !maybe_change_x &&
      i >= start_file &&
      i < start_file + max_disp_files
    )
    {
      ui::file_view().printEntry(
        start_file + cursor_pos,
        cursor_pos % window_height,
        cursor_pos / window_height,
        false,
        start_x
      );

      cursor_pos = static_cast<int>(i) - start_file;

      ui::file_view().printEntry(
        start_file + cursor_pos,
        cursor_pos % window_height,
        cursor_pos / window_height,
        true,
        start_x
      );
    } else {
      start_file = std::max(0, i - max_disp_files + 1);
      cursor_pos = i - start_file;

      ui::file_view().showFiles(start_file, start_file + cursor_pos, start_x, fe_ptr->Dir().get()->access_denied);
      maybe_change_x = false;
    }

    if (fe_ptr->Dir()->global_flag)
    {
      ui::file_view().showGlobalFileParameter(MakeGlobalFilePanelInfo(*fe_ptr));
    } else {
      ui::file_view().showFileParameter(MakeFilePanelInfo(*fe_ptr));
    }

    ui::file_view().refresh();
    ui::Present();

    const auto step = InvokeTaggedFileAction(action, *fe_ptr);
    continue_walk = step.continue_walk;

    if (step.replacement != nullptr && step.replacement != fe_ptr)
    {
      file_entry_list[i] = FindSharedFileEntry(step.replacement);
      ChangeFileEntry();
      max_disp_files = window_height * max_column;
      maybe_change_x = true;
    }
  }

}

template<typename F>
  requires std::invocable<F&, FileEntry&>
static void SilentWalkTaggedFiles(F&& action)
{
  for (const auto& fe_sp : file_entry_list)
  {
    auto fe_ptr = fe_sp.get();

    if (fe_ptr->tagged && fe_ptr->matching)
    {
      action(*fe_ptr);
    }
  }
}

template<typename F>
  requires std::invocable<F&, FileEntry&>
static void SilentTagWalkTaggedFiles(F&& action)
{
  for (const auto& fe_sp : file_entry_list)
  {
    auto fe_ptr = fe_sp.get();

    if (fe_ptr->tagged && fe_ptr->matching && action(*fe_ptr))
    {
      fe_ptr->tagged = false;
    }
  }
}

struct FileWindowContext
{
  DirEntry* dir_entry = nullptr;
  FileEntry* fe_ptr = nullptr;
  FileEntry* new_fe_ptr = nullptr;
  DirEntry* de_ptr = nullptr;
  DirEntry* dest_dir_entry = nullptr;
  std::optional<ui::InputEvent> unput;
  int tmp2 = 0;
  int list_pos = 0;
  std::int64_t file_size = 0;
  int i = 0;
  int start_x = 0;
  std::string modus;
  bool path_copy = false;
  int term = 0;
  int mask = 0;
  std::string to_dir;
  std::string to_path;
  std::string to_file;
  bool need_dsp_help = true;
  bool maybe_change_x_step = true;
  int dir_window_width = 0;
  int dir_window_height = 0;
};

static inline const std::shared_ptr<FileEntry>& FileWindowCurrentEntry(const DirEntry* dir_entry)
{
  return file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
}

static void InitializeFileWindow(FileWindowContext& ctx)
{
  ctx.unput.reset();
  ctx.fe_ptr = nullptr;
  ctx.need_dsp_help = true;
  ctx.maybe_change_x_step = true;

  BuildFileEntryList(ctx.dir_entry);

  if (ctx.dir_entry->cursor_pos < 0)
  {
    ctx.dir_entry->cursor_pos = 0;
  }
  if (
    !file_entry_list.empty() &&
    ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos >=
      static_cast<int>(file_entry_list.size())
  )
  {
    ctx.dir_entry->start_file = 0;
    ctx.dir_entry->cursor_pos = 0;
  }

  if (
    ctx.dir_entry->global_flag ||
    ctx.dir_entry->big_window ||
    ctx.dir_entry->tagged_flag
  )
  {
    ui::file_view().switchToBig();
    ui::file_view().getSize(window_height, window_width);
    ui::file_view().showDiskStatistic();
  }
  else
  {
    ui::file_view().getSize(window_height, window_width);
    ui::file_view().showDirStatistic(MakeDirPanelInfo(*ctx.dir_entry));
  }

  ui::file_view().showFilesAtCursor(ctx.dir_entry->start_file, ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
}

static void UpdateFileWindowLayout(FileWindowContext& ctx)
{
  if (!ctx.maybe_change_x_step)
  {
    return;
  }

  ctx.maybe_change_x_step = false;
  x_step = (max_column > 1) ? window_height : 1;
  max_disp_files = window_height * max_column;
}

static ui::InputEvent ReadFileWindowKey(FileWindowContext& ctx)
{
  if (ctx.unput)
  {
    const auto ev = *ctx.unput;
    ctx.unput.reset();
    return ev;
  }

  ctx.fe_ptr = FileWindowCurrentEntry(ctx.dir_entry).get();
  if (ctx.dir_entry->global_flag)
  {
    ui::file_view().showGlobalFileParameter(MakeGlobalFilePanelInfo(*ctx.fe_ptr));
  }
  else
  {
    ui::file_view().showFileParameter(MakeFilePanelInfo(*ctx.fe_ptr));
  }

  ui::file_view().refreshDirSide();
  ui::file_view().refresh();
  ui::Present();

  return ui::ReadEvent();
}

static void HandleFileWindowResize(FileWindowContext& ctx)
{
  if (!resize_request)
  {
    return;
  }

  ui::Relayout();
  RereadWindowSize(ctx.dir_entry);
  ui::DisplayMenu();

  ui::file_view().getDirPanelSize(ctx.dir_window_height, ctx.dir_window_width);
  while (statistic.cursor_pos >= ctx.dir_window_height)
  {
    statistic.cursor_pos--;
    statistic.disp_begin_pos++;
  }

  if (
    ctx.dir_entry->global_flag ||
    ctx.dir_entry->big_window ||
    ctx.dir_entry->tagged_flag
  )
  {
    ui::file_view().switchToBig();
    DisplayFileWindow(ctx.dir_entry);

    if (ctx.dir_entry->global_flag)
    {
      ui::file_view().showDiskStatistic();
      ui::file_view().showGlobalFileParameter(MakeGlobalFilePanelInfo(*ctx.fe_ptr));
    }
    else
    {
      DisplayFileWindow(ctx.dir_entry);
      ui::file_view().showDirStatistic(MakeDirPanelInfo(*ctx.dir_entry));
      ui::file_view().showFileParameter(MakeFilePanelInfo(*ctx.fe_ptr));
    }
  }
  else
  {
    ui::file_view().switchToSmall();
    ui::file_view().showDirTree(statistic.disp_begin_pos,
      statistic.disp_begin_pos + statistic.cursor_pos
    );
    DisplayFileWindow(ctx.dir_entry);
    ui::file_view().showDirStatistic(MakeDirPanelInfo(*ctx.dir_entry));
    ui::file_view().showFileParameter(MakeFilePanelInfo(*ctx.fe_ptr));
  }

  ctx.need_dsp_help = true;
  ui::DisplayAvailBytes();
  ui::DisplayFileSpec();
  ui::DisplayDiskName();
  resize_request = false;
}

static void RemapFileModeNavigationKeys(ui::InputEvent& ev)
{
  if (file_mode == ViewMode::MODE_1)
  {
    if (ev.key == ui::Key::Tab)
    {
      ev = ui::InputEvent::Special(ui::Key::Down);
    }
    else if (ev.key == ui::Key::BackTab)
    {
      ev = ui::InputEvent::Special(ui::Key::Up);
    }
  }
}

static void ResetFileWindowHorizontalScroll(
  const ui::InputEvent& ev,
  FileWindowContext& ctx
)
{
  if (
    x_step == 1 &&
    (ev.key == ui::Key::Right || ev.key == ui::Key::Left)
  )
  {
    return;
  }

  if (ctx.start_x == 0)
  {
    return;
  }

  ctx.start_x = 0;
  ui::file_view().printEntry(
    ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
    ctx.dir_entry->cursor_pos % window_height,
    ctx.dir_entry->cursor_pos / window_height,
    true,
    ctx.start_x
  );
}

static void ApplyFileUserMode(ui::InputEvent& ev, const FileWindowContext& ctx)
{
  if (mode == Mode::USER_MODE)
  {
    ev = FileUserMode(FileWindowCurrentEntry(ctx.dir_entry), ev);
  }
}

static void ProcessFileWindowKey(ui::InputEvent& ev, FileWindowContext& ctx)
{
  const FileCommand cmd = MapFileCommand(ev);

  switch (cmd)
  {
      case FileCommand::Resize:
        resize_request = true;
        break;

      case FileCommand::Mouse:
      {
        if (const auto injected = HandleFileMouse(
              &ctx.dir_entry->start_file,
              &ctx.dir_entry->cursor_pos,
              &ctx.start_x,
              ctx.dir_entry
            ))
        {
          ctx.unput = *injected;
        }
        break;
      }

      case FileCommand::MoveDown:
        MoveDown(
          ctx.dir_entry->start_file,
          ctx.dir_entry->cursor_pos,
          ctx.start_x,
          ctx.dir_entry
        );
        break;

      case FileCommand::MoveUp:
        MoveUp(
          ctx.dir_entry->start_file,
          ctx.dir_entry->cursor_pos,
          ctx.start_x,
          ctx.dir_entry
        );
        break;

      case FileCommand::MoveRight:
        MoveRight(
          &ctx.dir_entry->start_file,
          &ctx.dir_entry->cursor_pos,
          &ctx.start_x,
          ctx.dir_entry
        );
        break;

      case FileCommand::MoveLeft:
        MoveLeft(
          &ctx.dir_entry->start_file,
          &ctx.dir_entry->cursor_pos,
          &ctx.start_x,
          ctx.dir_entry
        );
        break;

      case FileCommand::PageDown:
        fmovenpage(
          &ctx.dir_entry->start_file,
          &ctx.dir_entry->cursor_pos,
          &ctx.start_x,
          ctx.dir_entry
        );
        break;

      case FileCommand::PageUp:
        fmoveppage(
          &ctx.dir_entry->start_file,
          &ctx.dir_entry->cursor_pos,
          &ctx.start_x,
          ctx.dir_entry
        );
        break;

      case FileCommand::End:
        if (
          ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos + 1 >=
          static_cast<int>(file_entry_list.size())
        )
        {
          ui::Beep();
        }
        else
        {
          if (static_cast<int>(file_entry_list.size()) < max_disp_files)
          {
            ctx.dir_entry->start_file = 0;
            ctx.dir_entry->cursor_pos =
              static_cast<int>(file_entry_list.size()) - 1;
          }
          else
          {
            ctx.dir_entry->start_file =
              static_cast<int>(file_entry_list.size()) - max_disp_files;
            ctx.dir_entry->cursor_pos =
              static_cast<int>(file_entry_list.size()) -
              ctx.dir_entry->start_file - 1;
          }

          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
        }
        break;

      case FileCommand::Home:
        if (ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos <= 0)
        {
          ui::Beep();
        }
        else
        {
          ctx.dir_entry->start_file = 0;
          ctx.dir_entry->cursor_pos = 0;
          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
        }
        break;

      case FileCommand::ListJump:
        ListJump(ctx.dir_entry, "");
        ctx.need_dsp_help = true;
        break;

      case FileCommand::Leave:
      case FileCommand::LoginEsc:
      case FileCommand::None:
        break;

      case FileCommand::ChangeMode:
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.need_dsp_help = true;
        if (!ctx.fe_ptr->ChangeModus())
        {
          ui::file_view().printEntry(
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.dir_entry->cursor_pos % window_height,
            ctx.dir_entry->cursor_pos / window_height,
            true,
            ctx.start_x
          );
        }
        break;

      case FileCommand::ChangeModeTagged:
          if( (mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      ui::Beep();
          }
          else
          {
      ctx.need_dsp_help = true;

        ctx.mask = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

      ctx.modus = GetAttributes(ctx.mask);

            if( ui::EditFileMode(ctx.modus, "\r\033") )
      {
        WalkTaggedFiles(
          ctx.dir_entry->start_file,
          ctx.dir_entry->cursor_pos,
          [&](FileEntry& fe)
          {
            return fe.SetModus(ctx.modus);
          }
        );

        ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
      }
      else
      {
        ui::Beep();
      }
          }
          break;

      case FileCommand::ChangeOwner:
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.need_dsp_help = true;
        if (ctx.fe_ptr->ChangeOwner())
        {
          ui::file_view().printEntry(
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.dir_entry->cursor_pos % window_height,
            ctx.dir_entry->cursor_pos / window_height,
            true,
            ctx.start_x
          );
        }
        break;

      case FileCommand::ChangeOwnerTagged:
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      ui::Beep();
          }
          else
          {
      ctx.need_dsp_help = true;
            if (const auto owner = GetNewOwner(-1))
      {
        if (const auto owner_id_ptr = GetPasswdUid(*owner))
        {
          const auto new_owner_id = *owner_id_ptr;

          WalkTaggedFiles(
            ctx.dir_entry->start_file,
            ctx.dir_entry->cursor_pos,
            [&](FileEntry& fe)
            {
              return fe.SetOwner(new_owner_id);
            }
          );

          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
        } else {
          FormatMessage("Can't read Owner-ID:*{}", *owner);
        }
      }
          }
          break;

      case FileCommand::ChangeGroup:
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.need_dsp_help = true;
        if (ctx.fe_ptr->ChangeGroup())
        {
          ui::file_view().printEntry(
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.dir_entry->cursor_pos % window_height,
            ctx.dir_entry->cursor_pos / window_height,
            true,
            ctx.start_x
          );
        }
        break;

      case FileCommand::ChangeGroupTagged:
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      ui::Beep();
          }
          else
          {
      ctx.need_dsp_help = true;

            if (const auto group = GetNewGroup(-1))
      {
        if (const auto group_id_ptr = GetGroupId(*group))
        {
          const auto new_group_id = *group_id_ptr;

          WalkTaggedFiles(
            ctx.dir_entry->start_file,
            ctx.dir_entry->cursor_pos,
            [&](FileEntry& fe)
            {
              return fe.SetGroup(new_group_id);
            }
          );

          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
        } else {
          FormatMessage("Can't read Group-ID:*\"{}\"", *group);
        }
      }
          }
          break;

      case FileCommand::Tag:      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          if( !ctx.fe_ptr->tagged )
          {
                        ctx.fe_ptr->tagged = true;

      ui::file_view().printEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.dir_entry->cursor_pos % window_height,
                ctx.dir_entry->cursor_pos / window_height,
                true,
          ctx.start_x
);
                  ctx.de_ptr->tagged_files++;
            ctx.de_ptr->tagged_bytes += ctx.fe_ptr->meta.size;
                  statistic.disk_tagged_files++;
            statistic.disk_tagged_bytes += ctx.fe_ptr->meta.size;
      if( ctx.dir_entry->global_flag )
        ui::file_view().showDiskTagged();
      else
        ui::file_view().showDirTagged(MakeTaggedCounts(*ctx.de_ptr));
          }
          ctx.unput = ui::InputEvent::Special(ui::Key::Down);

                      break;
      case FileCommand::Untag:      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();
                      if( ctx.fe_ptr->tagged )
          {
      ctx.fe_ptr->tagged = false;

      ui::file_view().printEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.dir_entry->cursor_pos % window_height,
                ctx.dir_entry->cursor_pos / window_height,
                true,
          ctx.start_x
);

      ctx.de_ptr->tagged_files--;
      ctx.de_ptr->tagged_bytes -= ctx.fe_ptr->meta.size;
      statistic.disk_tagged_files--;
      statistic.disk_tagged_bytes -= ctx.fe_ptr->meta.size;
      if( ctx.dir_entry->global_flag )
        ui::file_view().showDiskTagged();
      else
        ui::file_view().showDirTagged(MakeTaggedCounts(*ctx.de_ptr));
          }

          ctx.unput = ui::InputEvent::Special(ui::Key::Down);

          break;

      case FileCommand::RotateMode:
          ctx.list_pos = ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos;

          RotateFileMode();

                      x_step =  (max_column > 1) ? window_height : 1;
                      max_disp_files = window_height * max_column;

          if( ctx.dir_entry->cursor_pos >= max_disp_files )
          {
      /* Cursor muss neu positioniert werden */
      /*-------------------------------------*/

                        ctx.dir_entry->cursor_pos = max_disp_files - 1;
          }

          ctx.dir_entry->start_file = ctx.list_pos - ctx.dir_entry->cursor_pos;
          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
          break;

      case FileCommand::TagAll:
                      for(ctx.i=0; ctx.i < (int)file_entry_list.size(); ctx.i++)
                      {
      ctx.fe_ptr = file_entry_list[ctx.i].get();
      ctx.de_ptr = ctx.fe_ptr->Dir().get();

      if( !ctx.fe_ptr->tagged )
      {
        ctx.file_size = ctx.fe_ptr->meta.size;

        ctx.fe_ptr->tagged = true;
        ctx.de_ptr->tagged_files++;
        ctx.de_ptr->tagged_bytes += ctx.file_size;
        statistic.disk_tagged_files++;
        statistic.disk_tagged_bytes += ctx.file_size;
            }
          }

          if( ctx.dir_entry->global_flag )
            ui::file_view().showDiskTagged();
          else
            ui::file_view().showDirTagged(MakeTaggedCounts(*ctx.dir_entry));

          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
          break;


      case FileCommand::UntagAll:
                      for(ctx.i=0; ctx.i < (int)file_entry_list.size(); ctx.i++)
                      {
      ctx.fe_ptr = file_entry_list[ctx.i].get();
      ctx.de_ptr = ctx.fe_ptr->Dir().get();

      if( ctx.fe_ptr->tagged )
      {
        ctx.file_size = ctx.fe_ptr->meta.size;

        ctx.fe_ptr->tagged = false;
        ctx.de_ptr->tagged_files--;
        ctx.de_ptr->tagged_bytes -= ctx.file_size;
        statistic.disk_tagged_files--;
        statistic.disk_tagged_bytes -= ctx.file_size;
            }
          }

          if( ctx.dir_entry->global_flag )
            ui::file_view().showDiskTagged();
          else
            ui::file_view().showDirTagged(MakeTaggedCounts(*ctx.dir_entry));

          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
          break;



      case FileCommand::TagFromHere:
                      for(ctx.i=ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos; ctx.i < (int)file_entry_list.size(); ctx.i++)
                      {
      ctx.fe_ptr = file_entry_list[ctx.i].get();
      ctx.de_ptr = ctx.fe_ptr->Dir().get();

      if( !ctx.fe_ptr->tagged )
      {
        ctx.file_size = ctx.fe_ptr->meta.size;

        ctx.fe_ptr->tagged = true;
        ctx.de_ptr->tagged_files++;
        ctx.de_ptr->tagged_bytes += ctx.file_size;
        statistic.disk_tagged_files++;
        statistic.disk_tagged_bytes += ctx.file_size;
            }
          }

          if( ctx.dir_entry->global_flag )
            ui::file_view().showDiskTagged();
          else
            ui::file_view().showDirTagged(MakeTaggedCounts(*ctx.dir_entry));

          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
          break;


      case FileCommand::UntagFromHere:
                      for(ctx.i=ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos; ctx.i < (int)file_entry_list.size(); ctx.i++)
                      {
      ctx.fe_ptr = file_entry_list[ctx.i].get();
      ctx.de_ptr = ctx.fe_ptr->Dir().get();

      if( ctx.fe_ptr->tagged )
      {
        ctx.file_size = ctx.fe_ptr->meta.size;

        ctx.fe_ptr->tagged = false;
        ctx.de_ptr->tagged_files--;
        ctx.de_ptr->tagged_bytes -= ctx.file_size;
        statistic.disk_tagged_files--;
        statistic.disk_tagged_bytes -= ctx.file_size;
            }
          }

          if( ctx.dir_entry->global_flag )
            ui::file_view().showDiskTagged();
          else
            ui::file_view().showDirTagged(MakeTaggedCounts(*ctx.dir_entry));

          ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
          break;

      case FileCommand::View:
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.de_ptr = ctx.fe_ptr->Dir().get();
        View(ctx.dir_entry, ctx.fe_ptr->GetRealPath());
        ctx.need_dsp_help = true;
        break;

      case FileCommand::HexView:
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.de_ptr = ctx.fe_ptr->Dir().get();
        ViewHex(ctx.fe_ptr->GetRealPath());
        ctx.need_dsp_help = true;
        break;

      case FileCommand::Edit:
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.de_ptr = ctx.fe_ptr->Dir().get();
        Edit(ctx.de_ptr, ctx.fe_ptr->GetPath());
        break;

      case FileCommand::Copy:      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          ctx.path_copy = false;
          if (IsPathCopyKey(ev)) ctx.path_copy = true;

          ctx.need_dsp_help = true;

          {
            std::string copy_as;
            std::string copy_to;

            if (const auto copy_target = GetCopyParameter(ctx.fe_ptr->name, ctx.path_copy))
            {
              copy_as = copy_target->to_file;
              copy_to = copy_target->to_dir;
            } else {
              ui::Beep();
              break;
            }
            ctx.to_file = copy_as;
            ctx.to_dir = copy_to;
          }

          if( mode == Mode::DISK_MODE || mode == Mode::USER_MODE )
          {
                        if( (ctx.tmp2 = GetDirEntry(statistic.tree,
                 ctx.de_ptr,
                 ctx.to_dir,
                 &ctx.dest_dir_entry,
                 ctx.to_path
)) == -3)
                        {
                        }
                        else if (ctx.tmp2 != 0)
            {
        /* ui::Beep(); */
        break;
            }

            if( CopyFile(&statistic,
               ctx.fe_ptr,
               true,
               ctx.to_file,
               ctx.dest_dir_entry,
               ctx.to_path,
               ctx.path_copy
) )
            {
        /* File wurde kopiert */
        /*--------------------*/

                          ui::DisplayAvailBytes();

        if( ctx.dest_dir_entry )
        {
          /* Ziel befindet sich im SUB-Tree */
          /*--------------------------------*/

          if( ctx.dir_entry->global_flag )
            ui::file_view().showDiskStatistic();
          else
            ui::file_view().showDirStatistic(MakeDirPanelInfo(*ctx.de_ptr));

          if( ctx.dest_dir_entry == ctx.de_ptr )
          {
            /* Ziel ist aktuelles Verzeichnis */
            /*--------------------------------*/

            BuildFileEntryList(ctx.dir_entry);

            ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
          }
        }
            }
          }
          else
          {
      /* Mode::TAR_FILE_MODE */
      /*---------------*/

      ctx.dest_dir_entry = nullptr;

      if( disk_statistic.tree )
      {
        if( GetDirEntry(disk_statistic.tree,
                   ctx.de_ptr,
                   ctx.to_dir,
                   &ctx.dest_dir_entry,
                   ctx.to_path
) )
              {
          ui::Beep();
          break;
              }
            }
      else
      {
        ctx.to_path = ctx.to_dir;
      }
            if( CopyFile(&disk_statistic,
               ctx.fe_ptr,
               true,
               ctx.to_file,
               ctx.dest_dir_entry,
               ctx.to_path,
               ctx.path_copy
) )
            {
        /* File wurde kopiert */
        /*--------------------*/

                          ui::DisplayAvailBytes();
            }
          }
          break;

      case FileCommand::CopyTagged:
          ctx.de_ptr = ctx.dir_entry;

                      ctx.path_copy = false;
                      if (IsPathCopyKey(ev)) ctx.path_copy = true;

          if( !IsMatchingTaggedFiles() )
          {
      ui::Beep();
          }
          else
          {
            ctx.need_dsp_help = true;

      {
        std::string copy_as;
        std::string copy_to;

        if (const auto copy_target = GetCopyParameter(std::nullopt, ctx.path_copy))
        {
          copy_as = copy_target->to_file;
          copy_to = copy_target->to_dir;
        } else {
          ui::Beep();
          break;
        }
        ctx.to_file = copy_as;
        ctx.to_dir = copy_to;
      }


      if( mode == Mode::DISK_MODE || mode == Mode::USER_MODE )
      {
                          if( GetDirEntry(statistic.tree,
             ctx.de_ptr,
                   ctx.to_dir,
                   &ctx.dest_dir_entry,
                   ctx.to_path
) )
              {
          ui::Beep();
          break;
              }

        ctx.term = InputChoise("Confirm overwrite existing files (Y/N) ? ", "YN\033");
                          if( ctx.term == ESC )
              {
          ui::Beep();
          break;
        }

        const bool confirm = (ctx.term == 'Y');
        WalkTaggedFiles(
          ctx.dir_entry->start_file,
          ctx.dir_entry->cursor_pos,
          [&](FileEntry& fe)
          {
            const auto new_name = BuildFilename(fe.name, ctx.to_file);

            if (new_name.empty())
            {
              Message("Can't copy file to*empty name");

              return false;
            }

            return CopyFile(
              &statistic,
              &fe,
              confirm,
              new_name,
              ctx.dest_dir_entry,
              ctx.to_path,
              ctx.path_copy
            );
          }
        );

                          ui::DisplayAvailBytes();


        ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
            }
            else
            {
        /* Mode::TAR_FILE_MODE */
        /*---------------*/

        ctx.dest_dir_entry = nullptr;

        if( disk_statistic.tree )
        {
                            if( GetDirEntry(disk_statistic.tree,
               ctx.de_ptr,
                     ctx.to_dir,
                     &ctx.dest_dir_entry,
                     ctx.to_path
) )
                {
            ui::Beep();
            break;
                }
                    }
        else
        {
          ctx.to_path = ctx.to_dir;
        }

        ctx.term = InputChoise("Confirm overwrite existing files (Y/N) ? ", "YN\033");
                          if( ctx.term == ESC )
              {
          ui::Beep();
          break;
        }

        const bool confirm = (ctx.term == 'Y');
        WalkTaggedFiles(
          ctx.dir_entry->start_file,
          ctx.dir_entry->cursor_pos,
          [&](FileEntry& fe)
          {
            const auto new_name = BuildFilename(fe.name, ctx.to_file);

            if (new_name.empty())
            {
              Message("Can't copy file to*empty name");

              return false;
            }

            return CopyFile(
              &disk_statistic,
              &fe,
              confirm,
              new_name,
              ctx.dest_dir_entry,
              ctx.to_path,
              ctx.path_copy
            );
          }
        );

                          ui::DisplayAvailBytes();

        ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
            }
          }
          break;

      case FileCommand::Move:      if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
                      {
      ui::Beep();
      break;
          }

          ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          ctx.need_dsp_help = true;

          {
            std::string move_as;
            std::string move_to;

            if (const auto move_target = GetMoveParameter(ctx.fe_ptr->name))
            {
              move_as = move_target->to_file;
              move_to = move_target->to_dir;
            } else {
              ui::Beep();
              break;
            }
            ctx.to_file = move_as;
            ctx.to_dir = move_to;
          }

                      if( GetDirEntry(statistic.tree,
               ctx.de_ptr,
               ctx.to_dir,
               &ctx.dest_dir_entry,
               ctx.to_path
) )
          {
      ui::Beep();
      break;
          }

          if( MoveFile(ctx.fe_ptr,
             true,
             ctx.to_file,
             ctx.dest_dir_entry,
             ctx.to_path,
             &ctx.new_fe_ptr
) )
          {
      /* File wurde bewegt */
      /*-------------------*/

                        ui::DisplayAvailBytes();

      if( ctx.dir_entry->global_flag )
        ui::file_view().showDiskStatistic();
      else
        ui::file_view().showDirStatistic(MakeDirPanelInfo(*ctx.de_ptr));

      BuildFileEntryList(ctx.dir_entry);

      if( file_entry_list.size() == 0 ) ctx.unput = ui::InputEvent::Special(ui::Key::Escape);

      if( ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos >= (int)file_entry_list.size() )
      {
        if( --ctx.dir_entry->cursor_pos < 0 )
        {
          if( ctx.dir_entry->start_file > 0 )
          {
            ctx.dir_entry->start_file--;
          }
          ctx.dir_entry->cursor_pos = 0;
        }
      }

      ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
      ctx.maybe_change_x_step = true;
          }
          break;

      case FileCommand::MoveTagged:
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      ui::Beep();
          }
          else
          {
            ctx.need_dsp_help = true;

      {
        std::string move_as;
        std::string move_to;

        if (const auto move_target = GetMoveParameter(std::nullopt))
        {
          move_as = move_target->to_file;
          move_to = move_target->to_dir;
        } else {
          ui::Beep();
          break;
        }
        ctx.to_file = move_as;
        ctx.to_dir = move_to;
      }


                        if( GetDirEntry(statistic.tree,
           ctx.de_ptr,
                 ctx.to_dir,
                 &ctx.dest_dir_entry,
                 ctx.to_path
) )
            {
        ui::Beep();
        break;
            }

      ctx.term = InputChoise("Confirm overwrite existing files (Y/N) ? ", "YN\033");
                        if( ctx.term == ESC )
            {
        ui::Beep();
        break;
      }

      const bool confirm = (ctx.term == 'Y');
      WalkTaggedFiles(
        ctx.dir_entry->start_file,
        ctx.dir_entry->cursor_pos,
        [&](FileEntry& fe) -> TaggedFileWalkResult
        {
          const auto new_name = BuildFilename(fe.name, ctx.to_file);

          if (new_name.empty())
          {
            Message("Can't move file to*empty name");

            return {false, nullptr};
          }

          FileEntry* replacement = nullptr;
          const bool ok = MoveFile(
            &fe,
            confirm,
            new_name,
            ctx.dest_dir_entry,
            ctx.to_path,
            &replacement
          );

          return {
            ok,
            (replacement != nullptr && replacement != &fe) ? replacement : nullptr
          };
        }
      );

      BuildFileEntryList(ctx.dir_entry);

      if( file_entry_list.size() == 0 ) ctx.unput = ui::InputEvent::Special(ui::Key::Escape);

      ctx.dir_entry->start_file = 0;
      ctx.dir_entry->cursor_pos = 0;

      ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
      ctx.maybe_change_x_step = true;
          }
          break;

      case FileCommand::Delete:      if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
          {
      ui::Beep();
      break;
          }

          ctx.term = InputChoise("Delete this file (Y/N) ? ",
            "YN\033"
);

          ctx.need_dsp_help = true;

          if( ctx.term != 'Y' ) break;

          ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          if (ctx.fe_ptr->Delete())
          {
            /* File wurde geloescht */
      /*----------------------*/

      if( ctx.dir_entry->global_flag )
        ui::file_view().showDiskStatistic();
      else
        ui::file_view().showDirStatistic(MakeDirPanelInfo(*ctx.de_ptr));

      ui::DisplayAvailBytes();

                        RemoveFileEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos);

      if( file_entry_list.size() == 0 ) ctx.unput = ui::InputEvent::Special(ui::Key::Escape);

      if( ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos >= (int)file_entry_list.size() )
      {
        if( --ctx.dir_entry->cursor_pos < 0 )
        {
          if( ctx.dir_entry->start_file > 0 )
          {
            ctx.dir_entry->start_file--;
          }
          ctx.dir_entry->cursor_pos = 0;
        }
      }

      ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
      ctx.maybe_change_x_step = true;
          }
                      break;

      case FileCommand::DeleteTagged:
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      ui::Beep();
          }
          else
          {
            ctx.need_dsp_help = true;
      DeleteTaggedFiles(max_disp_files);
      if( file_entry_list.size() == 0 ) ctx.unput = ui::InputEvent::Special(ui::Key::Escape);
      ctx.dir_entry->start_file = 0;
      ctx.dir_entry->cursor_pos = 0;
                        ui::DisplayAvailBytes();
      ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
      ctx.maybe_change_x_step = true;
          }
          break;

      case FileCommand::Rename:       if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
          {
      ui::Beep();
      break;
          }

          ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          if (const auto renamed = GetRenameParameter(&ctx.fe_ptr->name))
          {
      if( RenameFile(ctx.fe_ptr, *renamed, &ctx.new_fe_ptr) )
            {
        /* Rename OK */
        /*-----------*/

        /* Maybe structure has changed... */
        /*--------------------------------*/

        BuildFileEntryList(ctx.de_ptr);

        ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.de_ptr->access_denied);
        ctx.maybe_change_x_step = true;
                        }
          }
          ctx.need_dsp_help = true;
          break;

      case FileCommand::RenameTagged:
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      ui::Beep();
          }
          else
          {
            ctx.need_dsp_help = true;

      const auto tagged_rename = GetRenameParameter(nullptr);
      if (!tagged_rename)
      {
        ui::Beep();
        break;
      }

      const auto new_name_pattern = *tagged_rename;
      WalkTaggedFiles(
        ctx.dir_entry->start_file,
        ctx.dir_entry->cursor_pos,
        [&](FileEntry& fe)
        {
          const auto new_name = BuildFilename(fe.name, new_name_pattern);

          if (new_name.empty())
          {
            Message("Can't rename file to*empty name");

            return false;
          }

          FileEntry* unchanged = &fe;
          return RenameFile(&fe, new_name, &unchanged);
        }
      );

      BuildFileEntryList(ctx.dir_entry);

      if( file_entry_list.size() == 0 ) ctx.unput = ui::InputEvent::Special(ui::Key::Escape);

      ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);

      ctx.maybe_change_x_step = true;
          }
          break;

      case FileCommand::Sort:
        if (const auto sort = GetKindOfSort())
        {
          statistic.kind_of_sort = *sort;
        }

        ctx.dir_entry->start_file = 0;
        ctx.dir_entry->cursor_pos = 0;

        SortFileEntryList();

        ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
        ctx.need_dsp_help = true;
        break;

      case FileCommand::FileSpec:       if(ReadFileSpec()) {

            ctx.dir_entry->start_file = 0;
            ctx.dir_entry->cursor_pos = 0;

            BuildFileEntryList(ctx.dir_entry);

            ui::DisplayFileSpec();
            ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);

            if( ctx.dir_entry->global_flag )
              ui::file_view().showDiskStatistic();
            else
              ui::file_view().showDirStatistic(MakeDirPanelInfo(*ctx.dir_entry));

                        if( file_entry_list.size() == 0 ) ctx.unput = ui::InputEvent::Special(ui::Key::Escape);
            ctx.maybe_change_x_step = true;
                }
          ctx.need_dsp_help = true;
          break;

      case FileCommand::ChangeLogin:
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
        {
          auto new_login_path = ctx.fe_ptr->GetPath().string();

          if (const auto login_path = GetNewLoginPath(new_login_path))
          {
            if (LoginDisk(*login_path))
            {
              ctx.dir_entry->login_flag = true;
              ctx.unput = ui::InputEvent::CharKey(LOGIN_ESC);
            }
          }
          ctx.need_dsp_help = true;
        } else {
          ui::Beep();
        }
        break;

      case FileCommand::Pipe:
          ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();
          ctx.fe_ptr->Pipe();
          ctx.need_dsp_help = true;
          break;

      case FileCommand::PipeTagged:
          ctx.de_ptr = ctx.dir_entry;

          if (!IsMatchingTaggedFiles())
          {
            ui::Beep();
          }
          else if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
          {
            Message("ctx.i am sorry*^P not supported in Archive-mode");
          } else {
            std::string filepath;
            FILE* pipe_file;

            ctx.need_dsp_help = true;

            if (const auto pipe_command = GetPipeCommand(filepath))
            {
              filepath = *pipe_command;
            } else {
              ui::Beep();
              break;
            }

            if (!(pipe_file = popen(filepath.c_str(), "w")))
            {
              FormatMessage("execution of command*{}*failed", filepath);
              break;
            }

            WalkTaggedFiles(
              ctx.dir_entry->start_file,
              ctx.dir_entry->cursor_pos,
              [&](FileEntry& fe)
              {
                return fe.PipeTo(pipe_file);
              }
            );

            ui::file_view().forceFullRedraw();

            if (pclose(pipe_file))
            {
              Warning("pclose() failed");
            }

            GetAvailBytes(statistic.disk_space);
            ui::DisplayAvailBytes();

            ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
          }
          break;

      case FileCommand::Execute:      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();
          Execute(ctx.de_ptr, ctx.fe_ptr);
          ctx.need_dsp_help = true;
          break;

      case FileCommand::SearchTagged:
          if (!IsMatchingTaggedFiles())
          {
            ui::Beep();
          }
          else if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
          {
            Message("Feature not available in archives.");
          } else {
            std::string command_line;

            ctx.need_dsp_help = true;
            if (const auto search_command = GetSearchCommandLine())
            {
              command_line = *search_command;
              ui::session().suspend();

              SilentTagWalkTaggedFiles(
                [&](FileEntry& fe) { return ExecuteCommand(&fe, command_line); }
              );

              ui::HitReturnToContinue();
              ui::session().resume(true);
              ui::file_view().refresh();

              ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
            }
          }
          break;

      case FileCommand::ExecuteTagged:
          if( !IsMatchingTaggedFiles() )
          {
            ui::Beep();
          }
          else if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
          {
            Message("I am sorry*^X not supported in Archive-mode");
          } else {
            std::string command_line;

            ctx.need_dsp_help = true;
            if (const auto tagged_command = GetCommandLine(command_line))
            {
              command_line = *tagged_command;
              ui::session().suspend();
              SilentWalkTaggedFiles(
                [&](FileEntry& fe) { ExecuteCommand(&fe, command_line); }
              );
              ui::HitReturnToContinue();
              ui::session().resume(true);

              ui::file_view().showFiles(ctx.dir_entry->start_file, ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos, ctx.start_x, ctx.dir_entry->access_denied);
            }
          }
          break;

      case FileCommand::QuitTo:
                      ctx.need_dsp_help = true;
                      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
                      ctx.de_ptr = ctx.fe_ptr->Dir().get();
                      QuitTo(ctx.de_ptr);
                      break;

      case FileCommand::Quit:       ctx.need_dsp_help = true;
                      Quit();
          break;

      case FileCommand::Redraw:
          ui::file_view().forceFullRedraw();
          break;

      case FileCommand::Unknown:
      default:
          ui::Beep();
          break;
  }
}

static bool ShouldContinueFileWindow(const ui::InputEvent& ev)
{
  const FileCommand cmd = MapFileCommand(ev);
  return cmd != FileCommand::Leave && cmd != FileCommand::LoginEsc;
}

static void FinalizeFileWindow(FileWindowContext& ctx, const ui::InputEvent& ev)
{
  if (ctx.dir_entry->big_window)
  {
    ui::file_view().switchToSmall();
  }

  const bool login_esc = MapFileCommand(ev) == FileCommand::LoginEsc;
  if (!login_esc)
  {
    ctx.dir_entry->global_flag = false;
    ctx.dir_entry->tagged_flag = false;
    ctx.dir_entry->big_window = false;
  }
}

int HandleFileWindow(DirEntry* dir_entry)
{
  FileWindowContext ctx;
  ctx.dir_entry = dir_entry;

  InitializeFileWindow(ctx);

  ui::InputEvent ev{};
  do
  {
    UpdateFileWindowLayout(ctx);

    if (ctx.need_dsp_help)
    {
      ctx.need_dsp_help = false;
      ui::DisplayFileHelp();
    }

    ev = ReadFileWindowKey(ctx);
    HandleFileWindowResize(ctx);
    RemapFileModeNavigationKeys(ev);
    ResetFileWindowHorizontalScroll(ev, ctx);
    ApplyFileUserMode(ev, ctx);
    ProcessFileWindowKey(ev, ctx);
  } while (ShouldContinueFileWindow(ev));

  FinalizeFileWindow(ctx, ev);
  return WindowExitCode(ev);
}





static bool IsMatchingTaggedFiles()
{
  FileEntry *fe_ptr;
  int i;

  for( i=0; i < (int)file_entry_list.size(); i++)
  {
    fe_ptr = file_entry_list[i].get();

    if( fe_ptr->matching && fe_ptr->tagged )
      return( true );
  }

  return( false );
}






static bool DeleteTaggedFiles(int max_disp_files)
{
  FileEntry *fe_ptr;
  DirEntry  *de_ptr;
  int       i;
  int       start_file;
  int       cursor_pos;
  bool      deleted;
  bool      confirm;
  int       term;
  int       start_x = 0;
  bool      result = true;

  term = InputChoise("Confirm delete each file (Y/N) ? ", "YN\033");

  if( term == ESC ) return false;

  if( term == 'Y' ) confirm = true;
  else confirm = false;

  for( i=0; i < (int)file_entry_list.size() && result; )
  {
    deleted = false;

    fe_ptr = file_entry_list[i].get();
    de_ptr = fe_ptr->Dir().get();

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      start_file = std::max(0, i - max_disp_files + 1);
      cursor_pos = i - start_file;

      ui::file_view().showFiles(start_file, start_file + cursor_pos, start_x, de_ptr->access_denied);

      if( fe_ptr->Dir()->global_flag )
        ui::file_view().showGlobalFileParameter(MakeGlobalFilePanelInfo(*fe_ptr));
      else
        ui::file_view().showFileParameter(MakeFilePanelInfo(*fe_ptr));

      ui::file_view().refresh();
      ui::Present();

      if( confirm ) term = InputChoise("Delete this file (Y/N) ? ", "YN\033");
      else term = 'Y';

      if( term == ESC )
      {
        result = false;
  break;
      }

      if (term == 'Y')
      {
        if (fe_ptr->Delete())
        {
          // File wurde geloescht

          deleted = true;
          result = true;

          if (de_ptr->global_flag)
          {
            ui::file_view().showDiskStatistic();
          } else {
            ui::file_view().showDirStatistic(MakeDirPanelInfo(*de_ptr));
          }

          ui::DisplayAvailBytes();

          RemoveFileEntry(start_file + cursor_pos);
        } else {
          result = false;
        }
      }
    }
    if (!deleted)
    {
      i++;
    }
  }
  return result;
}




static void RereadWindowSize(DirEntry *dir_entry)
{
  SetFileMode(file_mode);
  x_step =  (max_column > 1) ? window_height : 1;
  max_disp_files = window_height * max_column;


  if( dir_entry->start_file + dir_entry->cursor_pos < (int)file_entry_list.size() )
  {
     while( dir_entry->cursor_pos >= max_disp_files )
     {
         dir_entry->start_file += x_step;
         dir_entry->cursor_pos -= x_step;
     }
   }
   return;
}




static void ListJump(DirEntry * dir_entry, const char *str)
{
  const auto incremental = GetBooleanProfileValue("LISTJUMPSEARCH");
  int i = 0, j = 0, n = 0, start_x = 0, tmp2 = 0;

  const auto maybe = ui::Prompt("Jump to:", str ? str : "");
  if (!maybe || maybe->empty())
  {
    return;
  }
  const int ic = std::tolower(static_cast<unsigned char>(maybe->back()));
  if (!std::isprint(ic))
  {
    ui::Beep();
    return;
  }

  n = static_cast<int>(std::strlen(str));
  std::string newStr(str);
  newStr += static_cast<char>(ic);

  tmp2 = (incremental && n == 0) ? 0 : dir_entry->start_file + dir_entry->cursor_pos;

  if (tmp2 == static_cast<int>(file_entry_list.size() - 1))
  {
    ui::Message("Last entry!");
    ui::Beep();
    return;
  }

  FileEntry* fe_ptr = nullptr;
  for (i = tmp2; i < static_cast<int>(file_entry_list.size()); i++)
  {
    fe_ptr = file_entry_list[i].get();
    if (!strncasecmp(newStr.c_str(), fe_ptr->name.c_str(), n + 1))
      break;
  }

  if (i == static_cast<int>(file_entry_list.size()))
  {
    ui::Message("No match!");
    ui::Beep();
    return;
  }

  if (incremental && n == 0)
  {
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = 0;
    ui::file_view().showFiles(
      dir_entry->start_file,
      dir_entry->start_file + dir_entry->cursor_pos,
      start_x,
      dir_entry->access_denied
    );
  }
  for (j = tmp2; j < i; j++)
  {
    MoveDown(
      dir_entry->start_file,
      dir_entry->cursor_pos,
      start_x,
      dir_entry
    );
  }
  ui::file_view().refresh();
  ui::Present();
  ListJump(dir_entry, (incremental) ? newStr.c_str() : "");
}


const std::vector<std::shared_ptr<FileEntry>>& FileListEntries()
{
  return file_entry_list;
}

ViewMode GetFileViewMode()
{
  return file_mode;
}

int GetFileMaxColumn()
{
  return max_column;
}

int GetFileWindowHeight()
{
  return window_height;
}

int GetFileWindowWidth()
{
  return window_width;
}

int GetFileXStep()
{
  return x_step;
}

std::size_t GetFileMaxFilenameLen()
{
  return max_filename_len;
}

std::size_t GetFileMaxLinknameLen()
{
  return max_linkname_len;
}

unsigned GetFileMaxUserviewLen()
{
  return max_userview_len;
}

bool GetFileReverseSort()
{
  return reverse_sort;
}

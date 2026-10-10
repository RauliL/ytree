#include <algorithm>
#include <functional>

#include "./mouse.hpp"
#include "./walker.hpp"

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
static void DisplayFiles(DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x);
static void ReadGlobalFileList(const DirEntry* dir_entry);
static bool IsMatchingTaggedFiles();
static void RemoveFileEntry(int entry_no);
static void ChangeFileEntry();
static int  DeleteTaggedFiles(int max_dispfiles);
static void RereadWindowSize(DirEntry *dir_entry);
static void ListJump(DirEntry * dir_entry, const char *str);

void SetFileMode(ViewMode new_file_mode)
{

  GetMaxYX(file_window, &window_height, &window_width);
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
      if( S_ISLNK(fe_ptr->stat_struct.st_mode) )
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
      if (S_ISLNK(fe_ptr->stat_struct.st_mode))
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
     return (e1->stat_struct.st_mtime - e2->stat_struct.st_mtime) < 0;
  else
     return -(e1->stat_struct.st_mtime - e2->stat_struct.st_mtime) < 0;
}

static bool SortByChgTime(const FileEntry* e1, const FileEntry* e2)
{
  if (order)
     return (e1->stat_struct.st_ctime - e2->stat_struct.st_ctime) < 0;
  else
     return -(e1->stat_struct.st_ctime - e2->stat_struct.st_ctime) < 0;
}

static bool SortByAccTime(const FileEntry* e1, const FileEntry* e2)
{
  if (order)
     return (e1->stat_struct.st_atime - e2->stat_struct.st_atime) < 0;
  else
     return -(e1->stat_struct.st_atime - e2->stat_struct.st_atime) < 0;
}

static bool SortBySize(const FileEntry* e1, const FileEntry* e2)
{
  if (order)
     return (e1->stat_struct.st_size - e2->stat_struct.st_size) < 0;
  else
     return -(e1->stat_struct.st_size - e2->stat_struct.st_size) < 0;
}

static bool SortByOwner(const FileEntry* e1, const FileEntry* e2)
{
  std::string n1;
  std::string n2;

  if (const auto o1 = GetPasswdName(e1->stat_struct.st_uid))
  {
    n1 = *o1;
  } else {
    n1 = std::to_string(e1->stat_struct.st_uid);
  }
  if (const auto o2 = GetPasswdName(e2->stat_struct.st_uid))
  {
    n2 = *o2;
  } else {
    n2 = std::to_string(e2->stat_struct.st_uid);
  }
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
  std::string n1;
  std::string n2;

  if (const auto g1 = GetGroupName(e1->stat_struct.st_gid))
  {
    n1 = *g1;
  } else {
    n1 = std::to_string(e1->stat_struct.st_gid);
  }
  if (const auto g2 = GetGroupName(e2->stat_struct.st_gid))
  {
    n2 = *g2;
  } else {
    n2 = std::to_string(e2->stat_struct.st_gid);
  }
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

void SetKindOfSort(SortKey key, SortOrder order)
{
  statistic.kind_of_sort = { key, order };
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
    if (S_ISLNK(entry->stat_struct.st_mode))
    {
      max_linkname_len = std::max(
        max_linkname_len,
        static_cast<std::size_t>(StrVisualLength(entry->symlink_target))
);
    }
  }

  SetFileMode(file_mode); // Recalc.
}

char GetTypeOfFile(struct stat fst)
{
  if ( S_ISLNK(fst.st_mode) )
    return '@';
  else if ( S_ISSOCK(fst.st_mode) )
    return '=';
  else if ( S_ISCHR(fst.st_mode) )
    return '-';
  else if ( S_ISBLK(fst.st_mode) )
    return '+';
  else if ( S_ISFIFO(fst.st_mode) )
    return '|';
  else if ( S_ISREG(fst.st_mode) )
    return ' ';
  else
    return '?';
}


static void PrintFileEntry(int entry_no, int y, int x, unsigned char hilight, int start_x)
{
  char justify;
  char *line_ptr;
  int  n, pos_x = 0;
  FileEntry *fe_ptr;
  static std::string line_buffer;
  static int  old_cols = -1;
  int  ef_window_width;
  const char* sym_link_name = nullptr;
  char type_of_file = ' ';


  ef_window_width = window_width - 2; /* Effektive Window-Width */

  (reverse_sort) ? (justify='+') : (justify='-');

  if( old_cols != COLS )
  {
    old_cols = COLS;
    line_buffer.resize(COLS + PATH_LENGTH);
  }

  fe_ptr = file_entry_list[entry_no].get();

  if( fe_ptr && S_ISLNK(fe_ptr->stat_struct.st_mode) )
    sym_link_name = fe_ptr->symlink_target.c_str();
  else
    sym_link_name = "";


  type_of_file = fe_ptr ? GetTypeOfFile(fe_ptr->stat_struct) : ' ';

  const auto fitted_name = fe_ptr
    ? FitVisualWidth(fe_ptr->name, max_filename_len, justify == '-')
    : std::string{};
  const auto fitted_link = FitVisualWidth(sym_link_name, max_linkname_len, true);

  switch( file_mode )
  {
    case ViewMode::MODE_1 : if( fe_ptr )
      {
        const auto attributes = GetAttributes(fe_ptr->stat_struct.st_mode);
        const auto modify_time = CTime(fe_ptr->stat_struct.st_mtime);



                    if( S_ISLNK(fe_ptr->stat_struct.st_mode) )
        {
          line_buffer = std::format("{}{}{} {:>10} {:3} {:11} {:>12} -> {}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              attributes,
              fe_ptr->stat_struct.st_nlink,
                                      (std::int64_t) fe_ptr->stat_struct.st_size,
              modify_time,
              fitted_link
);
                    }
        else
        {
          line_buffer = std::format("{}{}{} {:>10} {:3} {:11} {:>12}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              attributes,
              fe_ptr->stat_struct.st_nlink,
                                      (std::int64_t) fe_ptr->stat_struct.st_size,
              modify_time
);
                    }
      }
      else
      {
        /* Empty Entry */
        /*-------------*/

        line_buffer = std::format("{:<{}}", "", max_filename_len + 42);
      }

      if( max_linkname_len )
        pos_x = x * (max_filename_len + max_linkname_len + 47);
      else
        pos_x = x * (max_filename_len + 43);
      break;

    case ViewMode::MODE_2 : if( fe_ptr )
      {
        const auto owner = GetPasswdName(fe_ptr->stat_struct.st_uid)
          .value_or(std::to_string(fe_ptr->stat_struct.st_uid));
        const auto group = GetGroupName(fe_ptr->stat_struct.st_gid)
          .value_or(std::to_string(fe_ptr->stat_struct.st_gid));

                    if( S_ISLNK(fe_ptr->stat_struct.st_mode) )
        {
                      line_buffer = std::format("{}{}{} {:10} {:<12} {:<12} -> {}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              (std::int64_t)fe_ptr->stat_struct.st_ino,
              owner,
              group,
              fitted_link
);
                    }
        else
        {
                      line_buffer = std::format("{}{}{} {:10} {:<12} {:<12}",
              (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
              type_of_file,
              fitted_name,
              (std::int64_t)fe_ptr->stat_struct.st_ino,
              owner,
              group
);

                    }
            }
      else
      {
        /* Empty-Entry */
        /*-------------*/

        line_buffer = std::format("{:<{}}", "", max_filename_len + 38);
      }

      if( max_linkname_len )
                    pos_x = x * (max_filename_len + max_linkname_len + 43);
      else
                    pos_x = x * (max_filename_len + 39);
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

        line_buffer = std::format("{:<{}}", "", max_filename_len + 2);
      }

      pos_x = x * (max_filename_len + 3);
      break;

    case ViewMode::MODE_4 : if( fe_ptr )
      {
        const auto change_time = CTime(fe_ptr->stat_struct.st_ctime);
        const auto access_time = CTime(fe_ptr->stat_struct.st_atime);

                    if( S_ISLNK(fe_ptr->stat_struct.st_mode) )
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

        line_buffer = std::format("{:<{}}", "", max_filename_len + 39);
      }


      if( max_linkname_len )
        pos_x = x * (max_filename_len + max_linkname_len + 44);
      else
        pos_x = x * (max_filename_len + 40);
      break;

    case ViewMode::MODE_5 : if( fe_ptr )
      {
        line_buffer.assign(std::max(old_cols + PATH_LENGTH, std::size_t{200}), '\0');
        BuildUserFileEntry(fe_ptr,  max_filename_len, max_linkname_len,
            GetProfileValueOrEmpty("USERVIEW"),
            200, line_buffer.data());
        line_buffer.resize(std::strlen(line_buffer.c_str()));
      }
      else
      {
        /* Empty-Entry */
        /*-------------*/

        line_buffer = std::format("{:<{}}", "", max_userview_len);
      }
      pos_x = x * (max_userview_len + 1);
      break;

  }

  /* display line */
  /*--------------*/

  n = StrVisualLength(line_buffer.c_str());

  if( n <= ef_window_width )
  {
    /* line fits */
    /*-----------*/

    hide_left = 0;
    hide_right = 0;
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
    hide_left = start_x;
    hide_right = n - start_x - ef_window_width;
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





void DisplayFileWindow(DirEntry *dir_entry)
{
  GetMaxYX(file_window, &window_height, &window_width);
  BuildFileEntryList(dir_entry);
  DisplayFiles(dir_entry, dir_entry->start_file,
                dir_entry->start_file + dir_entry->cursor_pos, 0);
}




static void DisplayFiles(DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x)
{
  int  x, y, p_x, p_y, j;

  werase(file_window);

  if( file_entry_list.size() == 0 )
  {
    mvwaddstr(file_window,
         0,
         3,
         (de_ptr->access_denied) ? "Permission Denied!" : "No Files!"
);
  }

  j = start_file_no; p_x = -1; p_y = 0;
  for( x=0; x < max_column; x++)
  {
    for( y=0; y < window_height; y++ )
    {
      if( j < (int)file_entry_list.size() )
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


static void fmovedown(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   if( *start_file + *cursor_pos + 1 >= (int)file_entry_list.size() )
   {
      /* File nicht vorhanden */
      /*----------------------*/
      beep();
   }
   else
   {
      if( *cursor_pos < max_disp_files - 1 )
      {
          /* DOWN ohne scroll moeglich */
          /*---------------------------*/
          PrintFileEntry(*start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          false,
                          *start_x
);
          (*cursor_pos)++;
          PrintFileEntry(*start_file + *cursor_pos,
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
          (*start_file)++;
          DisplayFiles(dir_entry,
                        *start_file,
                        *start_file + *cursor_pos,
                        *start_x
);
      }
   }
   return;
}

static void fmoveup(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   if( *start_file + *cursor_pos < 1 )
   {
      /* File nicht vorhanden */
      /*----------------------*/
      beep();
   }
   else
   {
      if( *cursor_pos > 0 )
      {
         /* UP ohne scroll moeglich */
         /*-------------------------*/
         PrintFileEntry(*start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         false,
                         *start_x
);
         (*cursor_pos)--;
         PrintFileEntry(*start_file + *cursor_pos,
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
         (*start_file)--;
         DisplayFiles(dir_entry,
                       *start_file,
                       *start_file + *cursor_pos,
                       *start_x
);
      }
   }
   return;
}

static void fmoveright(int *start_file, int *cursor_pos, int *start_x,DirEntry *dir_entry)
{
   if( x_step == 1 )
   {
      /* Sonderfall: ganzes Filewindow scrollen */
      /*----------------------------------------*/
      (*start_x)++;
      PrintFileEntry(*start_file + *cursor_pos,
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
      beep();
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
          PrintFileEntry(*start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          false,
                          *start_x
);
          *cursor_pos += my_x_step;
          PrintFileEntry(*start_file + *cursor_pos,
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
          DisplayFiles(dir_entry,
                        *start_file,
                        *start_file + *cursor_pos,
                        *start_x
);
      }
   }
   return;
}


static void fmoveleft(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
     if( x_step == 1 )
     {
         /* Sonderfall: ganzes Filewindow scrollen */
         /*----------------------------------------*/
         if( *start_x > 0 ) (*start_x)--;
            PrintFileEntry(*start_file + *cursor_pos,
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
         beep();
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
             PrintFileEntry(*start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             false,
                             *start_x
);
             *cursor_pos -= my_x_step;
             PrintFileEntry(*start_file + *cursor_pos,
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
             DisplayFiles(dir_entry,
                           *start_file,
                           *start_file + *cursor_pos,
                           *start_x
);
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
      beep();
   }
   else
   {
      if( *cursor_pos < max_disp_files - 1 )
      {
        /* Cursor steht noch nicht auf letztem
         * Eintrag
         * ==> setzen
         */
         PrintFileEntry(*start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         false,
                         *start_x
);
         if( *start_file + max_disp_files <= (int)file_entry_list.size() - 1 )
            *cursor_pos = max_disp_files - 1;
         else
            *cursor_pos = file_entry_list.size() - *start_file - 1;
         PrintFileEntry(*start_file + *cursor_pos,
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
        DisplayFiles(dir_entry,
                      *start_file,
                      *start_file + *cursor_pos,
                      *start_x
);
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
        beep();
     }
     else
     {
        if( *cursor_pos > 0 )
        {
            /* Cursor steht noch nicht auf erstem
             * Eintrag
             * ==> setzen
             */
             PrintFileEntry(*start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             false,
                             *start_x
);
             *cursor_pos = 0;
             PrintFileEntry(*start_file + *cursor_pos,
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
            DisplayFiles(dir_entry,
                          *start_file,
                          *start_file + *cursor_pos,
                          *start_x
);
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
    PrintFileEntry(*start_file + *cursor_pos,
                   *cursor_pos % window_height,
                   *cursor_pos / window_height,
                   false,
                   *start_x);
    *cursor_pos = target_pos - *start_file;
    PrintFileEntry(*start_file + *cursor_pos,
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
    DisplayFiles(dir_entry, *start_file, *start_file + *cursor_pos, *start_x);
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

static int HandleFileMouse(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
  const MouseEvent event = DecodeMouse(MouseFocus::File);

  switch (event.action)
  {
    case MouseAction::ScrollUp:
      return KEY_UP;
    case MouseAction::ScrollDown:
      return KEY_DOWN;
    case MouseAction::SwitchToDir:
      return ESC;
    case MouseAction::Select:
    case MouseAction::Activate:
    case MouseAction::Tag:
    {
      const int relative = FileIndexFromMouse(event.row, event.col);
      if (relative < 0)
      {
        return -1;
      }
      fmoveto(*start_file + relative, start_file, cursor_pos, start_x, dir_entry);
      if (event.action == MouseAction::Activate)
      {
        return 'v';
      }
      if (event.action == MouseAction::Tag)
      {
        return 't';
      }
      return -1;
    }
    case MouseAction::Ignore:
    case MouseAction::None:
    case MouseAction::SwitchToFile:
    default:
      return -1;
  }
}

template<typename Context>
  requires std::is_base_of_v<WalkContextBase, Context>
static void WalkTaggedFiles(
  int start_file,
  int cursor_pos,
  int (*fkt)(FileEntry*, Context*),
  Context* ctx
)
{
  FileEntry *fe_ptr;
  int       i;
  int       start_x = 0;
  int       result = 0;
  bool      maybe_change_x = false;

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead(0);

  max_disp_files = window_height * max_column;

  for( i=0; i < (int)file_entry_list.size() && result == 0; i++ )
  {
    const auto fe_sp = file_entry_list[i];

    fe_ptr = fe_sp.get();

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      if( maybe_change_x == false &&
    i >= start_file && i < start_file + max_disp_files )
      {
    PrintFileEntry(start_file + cursor_pos,
      cursor_pos % window_height,
      cursor_pos / window_height,
      false,
            start_x
);

        cursor_pos = i - start_file;

  PrintFileEntry(start_file + cursor_pos,
      cursor_pos % window_height,
      cursor_pos / window_height,
      true,
            start_x
);
      }
      else
      {
  start_file = std::max(0, i - max_disp_files + 1);
  cursor_pos = i - start_file;

        DisplayFiles(fe_ptr->Dir().get(),
          start_file,
          start_file + cursor_pos,
          start_x
);
  maybe_change_x = false;
      }

      if( fe_ptr->Dir()->global_flag )
        DisplayGlobalFileParameter(fe_ptr);
      else
        DisplayFileParameter(fe_ptr);

      RefreshWindow(file_window);
      doupdate();
      result = fkt(fe_ptr, ctx);
      if( ctx->new_fe_ptr != fe_ptr )
      {
        file_entry_list[i] = FindSharedFileEntry(ctx->new_fe_ptr);
  ChangeFileEntry();
        max_disp_files = window_height * max_column;
  maybe_change_x = true;
      }
    }
  }

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead(-1);
}

template<typename Context>
  requires std::is_base_of_v<WalkContextBase, Context>
static void SilentWalkTaggedFiles(
  int (*fkt)(FileEntry*, Context*),
  Context* ctx
)
{
  for( int i = 0; i < (int)file_entry_list.size(); i++ )
  {
    FileEntry *fe_ptr = file_entry_list[i].get();

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      fkt(fe_ptr, ctx);
    }
  }
}

template<typename Context>
  requires std::is_base_of_v<WalkContextBase, Context>
static void SilentTagWalkTaggedFiles(
  int (*fkt)(FileEntry*, Context*),
  Context* ctx
)
{
  for( int i = 0; i < (int)file_entry_list.size(); i++ )
  {
    FileEntry *fe_ptr = file_entry_list[i].get();

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      const int result = fkt(fe_ptr, ctx);

      if( result == 0 ) {
        fe_ptr->tagged = false;
      }
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
  int unput_char = 0;
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

static FileEntry* FileWindowCurrentEntry(const DirEntry* dir_entry)
{
  return file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].get();
}

static void DisplayFilesAtCursor(DirEntry* dir_entry, int start_x)
{
  DisplayFiles(
    dir_entry,
    dir_entry->start_file,
    dir_entry->start_file + dir_entry->cursor_pos,
    start_x
  );
}

static void InitializeFileWindow(FileWindowContext& ctx)
{
  ctx.unput_char = '\0';
  ctx.fe_ptr = nullptr;
  ctx.need_dsp_help = true;
  ctx.maybe_change_x_step = true;

  BuildFileEntryList(ctx.dir_entry);

  if (
    ctx.dir_entry->global_flag ||
    ctx.dir_entry->big_window ||
    ctx.dir_entry->tagged_flag
  )
  {
    SwitchToBigFileWindow();
    GetMaxYX(file_window, &window_height, &window_width);
    DisplayDiskStatistic();
  }
  else
  {
    GetMaxYX(file_window, &window_height, &window_width);
    DisplayDirStatistic(ctx.dir_entry);
  }

  DisplayFilesAtCursor(ctx.dir_entry, ctx.start_x);
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

static int ReadFileWindowKey(FileWindowContext& ctx)
{
  if (ctx.unput_char)
  {
    const int ch = ctx.unput_char;
    ctx.unput_char = '\0';
    return ch;
  }

  ctx.fe_ptr = FileWindowCurrentEntry(ctx.dir_entry);
  if (ctx.dir_entry->global_flag)
  {
    DisplayGlobalFileParameter(ctx.fe_ptr);
  }
  else
  {
    DisplayFileParameter(ctx.fe_ptr);
  }

  RefreshWindow(dir_window);
  RefreshWindow(file_window);
  doupdate();

  int ch = resize_request ? -1 : Getch();
  if (ch == LF)
  {
    ch = CR;
  }

#ifdef VI_KEYS
  ch = ViKey(ch);
#endif

  return ch;
}

static void HandleFileWindowResize(FileWindowContext& ctx)
{
  if (!resize_request)
  {
    return;
  }

  ReCreateWindows();
  RereadWindowSize(ctx.dir_entry);
  DisplayMenu();

  GetMaxYX(dir_window, &ctx.dir_window_height, &ctx.dir_window_width);
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
    SwitchToBigFileWindow();
    DisplayFileWindow(ctx.dir_entry);

    if (ctx.dir_entry->global_flag)
    {
      DisplayDiskStatistic();
      DisplayGlobalFileParameter(ctx.fe_ptr);
    }
    else
    {
      DisplayFileWindow(ctx.dir_entry);
      DisplayDirStatistic(ctx.dir_entry);
      DisplayFileParameter(ctx.fe_ptr);
    }
  }
  else
  {
    SwitchToSmallFileWindow();
    DisplayTree(
      dir_window,
      statistic.disp_begin_pos,
      statistic.disp_begin_pos + statistic.cursor_pos
    );
    DisplayFileWindow(ctx.dir_entry);
    DisplayDirStatistic(ctx.dir_entry);
    DisplayFileParameter(ctx.fe_ptr);
  }

  ctx.need_dsp_help = true;
  DisplayAvailBytes();
  DisplayFileSpec();
  DisplayDiskName();
  resize_request = false;
}

static void RemapFileModeNavigationKeys(int& ch)
{
  if (file_mode == ViewMode::MODE_1)
  {
    if (ch == '\t')
    {
      ch = KEY_DOWN;
    }
    else if (ch == KEY_BTAB)
    {
      ch = KEY_UP;
    }
  }
}

static void ResetFileWindowHorizontalScroll(int ch, FileWindowContext& ctx)
{
  if (x_step == 1 && (ch == KEY_RIGHT || ch == KEY_LEFT))
  {
    return;
  }

  if (ctx.start_x == 0)
  {
    return;
  }

  ctx.start_x = 0;
  PrintFileEntry(
    ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
    ctx.dir_entry->cursor_pos % window_height,
    ctx.dir_entry->cursor_pos / window_height,
    true,
    ctx.start_x
  );
}

static void ApplyFileUserMode(int& ch, FileWindowContext& ctx)
{
  if (mode == Mode::USER_MODE)
  {
    ch = FileUserMode(FileWindowCurrentEntry(ctx.dir_entry), ch);
  }
}

static void ProcessFileWindowKey(int& ch, FileWindowContext& ctx)
{
  switch (ch)
  {

#ifdef KEY_RESIZE

      case KEY_RESIZE: resize_request = true;
                       break;
#endif

#ifdef KEY_MOUSE
      case KEY_MOUSE:  ch = HandleFileMouse(&ctx.dir_entry->start_file,
                                            &ctx.dir_entry->cursor_pos,
                                            &ctx.start_x,
                                            ctx.dir_entry);
                       if (ch == -1)
                         break;
                       ctx.unput_char = ch;
                       break;
#endif

      case -1:         break;

      case ' ' :   /*   break;  Quick-Key */

      case KEY_DOWN :  fmovedown(&ctx.dir_entry->start_file, &ctx.dir_entry->cursor_pos, &ctx.start_x, ctx.dir_entry);
          break;

      case KEY_UP   : fmoveup(&ctx.dir_entry->start_file, &ctx.dir_entry->cursor_pos, &ctx.start_x, ctx.dir_entry);
          break;

      case KEY_RIGHT: fmoveright(&ctx.dir_entry->start_file, &ctx.dir_entry->cursor_pos, &ctx.start_x, ctx.dir_entry);
          break;

      case KEY_LEFT : fmoveleft(&ctx.dir_entry->start_file, &ctx.dir_entry->cursor_pos, &ctx.start_x, ctx.dir_entry);
          break;

      case KEY_NPAGE: fmovenpage(&ctx.dir_entry->start_file, &ctx.dir_entry->cursor_pos, &ctx.start_x, ctx.dir_entry);
          break;

      case KEY_PPAGE: fmoveppage(&ctx.dir_entry->start_file, &ctx.dir_entry->cursor_pos, &ctx.start_x, ctx.dir_entry);
          break;

      case KEY_END  : if( ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos + 1 >= (int)file_entry_list.size() )
          {
      /* Letzte Position erreicht */
      /*--------------------------*/

      beep();
          }
          else
          {
      if( (int)file_entry_list.size() < max_disp_files )
            {
        ctx.dir_entry->start_file = 0;
        ctx.dir_entry->cursor_pos = file_entry_list.size() - 1;
            }
            else
                  {
                          ctx.dir_entry->start_file = file_entry_list.size() - max_disp_files;
        ctx.dir_entry->cursor_pos = file_entry_list.size() - ctx.dir_entry->start_file - 1;
            }

      DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);
          }
          break;

      case KEY_HOME : if( ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos <= 0 )
          {
      /* erste Position erreicht */
      /*-------------------------*/

      beep();
          }
          else
          {
                        ctx.dir_entry->start_file = 0;
      ctx.dir_entry->cursor_pos = 0;

      DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);

          }
          break;

      case 'A' :
      case 'a' :      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();

                ctx.need_dsp_help = true;

          if( !ChangeFileModus(ctx.fe_ptr) )
          {
      PrintFileEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.dir_entry->cursor_pos % window_height,
                ctx.dir_entry->cursor_pos / window_height,
                true,
          ctx.start_x
);
          }
          break;

      case 'A' & 0x1F :
          if( (mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      beep();
          }
          else
          {
      ctx.need_dsp_help = true;

        ctx.mask = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

      ctx.modus = GetAttributes(ctx.mask);

            if( GetNewFileModus(LINES - 2, 1, ctx.modus, "\r\033") )
      {
        ChangeModusWalkContext modus_ctx;
        modus_ctx.new_modus = ctx.modus;
                          WalkTaggedFiles(ctx.dir_entry->start_file,
             ctx.dir_entry->cursor_pos,
             SetFileModus,
             &modus_ctx
);

        DisplayFiles(ctx.dir_entry,
          ctx.dir_entry->start_file,
          ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
          ctx.start_x
);
      }
      else
      {
        beep();
      }
          }
          break;

      case 'O' :
      case 'o' :      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();

          ctx.need_dsp_help = true;

          if( !ChangeFileOwner(ctx.fe_ptr) )
          {
      PrintFileEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.dir_entry->cursor_pos % window_height,
                ctx.dir_entry->cursor_pos / window_height,
                true ,
          ctx.start_x
);
          }
          break;

      case 'O' & 0x1F :
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      beep();
          }
          else
          {
      ctx.need_dsp_help = true;
            if (const auto owner = GetNewOwner(-1))
      {
        if (const auto owner_id_ptr = GetPasswdUid(*owner))
        {
          ChangeOwnerWalkContext owner_ctx;
          owner_ctx.new_owner_id = *owner_id_ptr;
          WalkTaggedFiles(
            ctx.dir_entry->start_file,
            ctx.dir_entry->cursor_pos,
            SetFileOwner,
            &owner_ctx
          );

          DisplayFiles(
            ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
          );
        } else {
          FormatMessage("Can't read Owner-ID:*{}", *owner);
        }
      }
          }
          break;

      case 'G' :
      case 'g' :      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();

          ctx.need_dsp_help = true;

          if( !ChangeFileGroup(ctx.fe_ptr) )
          {
      PrintFileEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.dir_entry->cursor_pos % window_height,
                ctx.dir_entry->cursor_pos / window_height,
                true,
          ctx.start_x
);
          }
          break;

      case 'G' & 0x1F :
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      beep();
          }
          else
          {
      ctx.need_dsp_help = true;

            if (const auto group = GetNewGroup(-1))
      {
        if (const auto group_id_ptr = GetGroupId(*group))
        {
          ChangeGroupWalkContext group_ctx;
          group_ctx.new_group_id = *group_id_ptr;
          WalkTaggedFiles(
            ctx.dir_entry->start_file,
            ctx.dir_entry->cursor_pos,
            SetFileGroup,
            &group_ctx
          );

          DisplayFiles(
            ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
          );
        } else {
          FormatMessage("Can't read Group-ID:*\"{}\"", *group);
        }
      }
          }
          break;

      case 'T' :
      case 't' :      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          if( !ctx.fe_ptr->tagged )
          {
                        ctx.fe_ptr->tagged = true;

      PrintFileEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.dir_entry->cursor_pos % window_height,
                ctx.dir_entry->cursor_pos / window_height,
                true,
          ctx.start_x
);
                  ctx.de_ptr->tagged_files++;
            ctx.de_ptr->tagged_bytes += ctx.fe_ptr->stat_struct.st_size;
                  statistic.disk_tagged_files++;
            statistic.disk_tagged_bytes += ctx.fe_ptr->stat_struct.st_size;
      if( ctx.dir_entry->global_flag )
        DisplayDiskTagged();
      else
        DisplayDirTagged(ctx.de_ptr);
          }
          ctx.unput_char = KEY_DOWN;

                      break;
      case 'U' :
      case 'u' :      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();
                      if( ctx.fe_ptr->tagged )
          {
      ctx.fe_ptr->tagged = false;

      PrintFileEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.dir_entry->cursor_pos % window_height,
                ctx.dir_entry->cursor_pos / window_height,
                true,
          ctx.start_x
);

      ctx.de_ptr->tagged_files--;
      ctx.de_ptr->tagged_bytes -= ctx.fe_ptr->stat_struct.st_size;
      statistic.disk_tagged_files--;
      statistic.disk_tagged_bytes -= ctx.fe_ptr->stat_struct.st_size;
      if( ctx.dir_entry->global_flag )
        DisplayDiskTagged();
      else
        DisplayDirTagged(ctx.de_ptr);
          }

          ctx.unput_char = KEY_DOWN;

          break;

      case 'F' & 0x1F :
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
          DisplayFiles(ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
);
          break;

      case 'T' & 0x1F :
                      for(ctx.i=0; ctx.i < (int)file_entry_list.size(); ctx.i++)
                      {
      ctx.fe_ptr = file_entry_list[ctx.i].get();
      ctx.de_ptr = ctx.fe_ptr->Dir().get();

      if( !ctx.fe_ptr->tagged )
      {
        ctx.file_size = ctx.fe_ptr->stat_struct.st_size;

        ctx.fe_ptr->tagged = true;
        ctx.de_ptr->tagged_files++;
        ctx.de_ptr->tagged_bytes += ctx.file_size;
        statistic.disk_tagged_files++;
        statistic.disk_tagged_bytes += ctx.file_size;
            }
          }

          if( ctx.dir_entry->global_flag )
            DisplayDiskTagged();
          else
            DisplayDirTagged(ctx.dir_entry);

          DisplayFiles(ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
);
          break;


      case 'U' & 0x1F :
                      for(ctx.i=0; ctx.i < (int)file_entry_list.size(); ctx.i++)
                      {
      ctx.fe_ptr = file_entry_list[ctx.i].get();
      ctx.de_ptr = ctx.fe_ptr->Dir().get();

      if( ctx.fe_ptr->tagged )
      {
        ctx.file_size = ctx.fe_ptr->stat_struct.st_size;

        ctx.fe_ptr->tagged = false;
        ctx.de_ptr->tagged_files--;
        ctx.de_ptr->tagged_bytes -= ctx.file_size;
        statistic.disk_tagged_files--;
        statistic.disk_tagged_bytes -= ctx.file_size;
            }
          }

          if( ctx.dir_entry->global_flag )
            DisplayDiskTagged();
          else
            DisplayDirTagged(ctx.dir_entry);

          DisplayFiles(ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
);
          break;



      case ';':
      case 't' | 0x80 :
                      for(ctx.i=ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos; ctx.i < (int)file_entry_list.size(); ctx.i++)
                      {
      ctx.fe_ptr = file_entry_list[ctx.i].get();
      ctx.de_ptr = ctx.fe_ptr->Dir().get();

      if( !ctx.fe_ptr->tagged )
      {
        ctx.file_size = ctx.fe_ptr->stat_struct.st_size;

        ctx.fe_ptr->tagged = true;
        ctx.de_ptr->tagged_files++;
        ctx.de_ptr->tagged_bytes += ctx.file_size;
        statistic.disk_tagged_files++;
        statistic.disk_tagged_bytes += ctx.file_size;
            }
          }

          if( ctx.dir_entry->global_flag )
            DisplayDiskTagged();
          else
            DisplayDirTagged(ctx.dir_entry);

          DisplayFiles(ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
);
          break;


      case ':':
      case 'u' | 0x80 :
                      for(ctx.i=ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos; ctx.i < (int)file_entry_list.size(); ctx.i++)
                      {
      ctx.fe_ptr = file_entry_list[ctx.i].get();
      ctx.de_ptr = ctx.fe_ptr->Dir().get();

      if( ctx.fe_ptr->tagged )
      {
        ctx.file_size = ctx.fe_ptr->stat_struct.st_size;

        ctx.fe_ptr->tagged = false;
        ctx.de_ptr->tagged_files--;
        ctx.de_ptr->tagged_bytes -= ctx.file_size;
        statistic.disk_tagged_files--;
        statistic.disk_tagged_bytes -= ctx.file_size;
            }
          }

          if( ctx.dir_entry->global_flag )
            DisplayDiskTagged();
          else
            DisplayDirTagged(ctx.dir_entry);

          DisplayFiles(ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
);
          break;

      case 'V':
      case 'v':
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.de_ptr = ctx.fe_ptr->Dir().get();
        View(ctx.dir_entry, GetRealFileNamePath(ctx.fe_ptr));
        ctx.need_dsp_help = true;
        break;

      case 'H':
      case 'h':
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.de_ptr = ctx.fe_ptr->Dir().get();
        ViewHex(GetRealFileNamePath(ctx.fe_ptr));
        ctx.need_dsp_help = true;
        break;

      case 'E':
      case 'e':
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        ctx.de_ptr = ctx.fe_ptr->Dir().get();
        Edit(ctx.de_ptr, GetFileNamePath(ctx.fe_ptr));
        break;

      case 'Y' :
      case 'y' :
      case 'C' :
      case 'c' :      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          ctx.path_copy = false;
          if( ch == 'y' || ch == 'Y' ) ctx.path_copy = true;

          ctx.need_dsp_help = true;

          {
            std::string copy_as;
            std::string copy_to;

            if (const auto copy_target = GetCopyParameter(ctx.fe_ptr->name, ctx.path_copy))
            {
              copy_as = copy_target->to_file;
              copy_to = copy_target->to_dir;
            } else {
              beep();
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
        /* beep(); */
        break;
            }

            if( !CopyFile(&statistic,
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

                          DisplayAvailBytes();

        if( ctx.dest_dir_entry )
        {
          /* Ziel befindet sich im SUB-Tree */
          /*--------------------------------*/

          if( ctx.dir_entry->global_flag )
            DisplayDiskStatistic();
          else
            DisplayDirStatistic(ctx.de_ptr);

          if( ctx.dest_dir_entry == ctx.de_ptr )
          {
            /* Ziel ist aktuelles Verzeichnis */
            /*--------------------------------*/

            BuildFileEntryList(ctx.dir_entry);

            DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);
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
          beep();
          break;
              }
            }
      else
      {
        ctx.to_path = ctx.to_dir;
      }
            if( !CopyFile(&disk_statistic,
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

                          DisplayAvailBytes();
            }
          }
          break;

      case 'Y' & 0x1F :
      case 'K' & 0x1F :
      case 'C' & 0x1F :
          ctx.de_ptr = ctx.dir_entry;

                      ctx.path_copy = false;
                      if( ch == ('Y' & 0x1F) ) ctx.path_copy = true;

          if( !IsMatchingTaggedFiles() )
          {
      beep();
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
          beep();
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
          beep();
          break;
              }

        ctx.term = InputChoise("Confirm overwrite existing files (Y/N) ? ", "YN\033");
                          if( ctx.term == ESC )
              {
          beep();
          break;
        }

        CopyWalkContext copy_ctx;
        copy_ctx.statistic_ptr = &statistic;
        copy_ctx.dest_dir_entry = ctx.dest_dir_entry;
        copy_ctx.to_file = ctx.to_file;
        copy_ctx.to_path = ctx.to_path;
        copy_ctx.path_copy = ctx.path_copy;
        copy_ctx.confirm = (ctx.term == 'Y');

        WalkTaggedFiles(ctx.dir_entry->start_file,
             ctx.dir_entry->cursor_pos,
             CopyTaggedFiles,
             &copy_ctx
);

                          DisplayAvailBytes();


        DisplayFiles(ctx.dir_entry,
          ctx.dir_entry->start_file,
          ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
          ctx.start_x
);
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
            beep();
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
          beep();
          break;
        }

        CopyWalkContext copy_ctx;
        copy_ctx.statistic_ptr = &disk_statistic;
        copy_ctx.dest_dir_entry = ctx.dest_dir_entry;
        copy_ctx.to_file = ctx.to_file;
        copy_ctx.to_path = ctx.to_path;
        copy_ctx.path_copy = ctx.path_copy;
        copy_ctx.confirm = (ctx.term == 'Y');

        WalkTaggedFiles(ctx.dir_entry->start_file,
             ctx.dir_entry->cursor_pos,
             CopyTaggedFiles,
             &copy_ctx
);

                          DisplayAvailBytes();

        DisplayFiles(ctx.dir_entry,
          ctx.dir_entry->start_file,
          ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
          ctx.start_x
);
            }
          }
          break;

      case 'M' :
      case 'm' :      if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
                      {
      beep();
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
              beep();
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
      beep();
      break;
          }

          if( !MoveFile(ctx.fe_ptr,
             true,
             ctx.to_file,
             ctx.dest_dir_entry,
             ctx.to_path,
             &ctx.new_fe_ptr
) )
          {
      /* File wurde bewegt */
      /*-------------------*/

                        DisplayAvailBytes();

      if( ctx.dir_entry->global_flag )
        DisplayDiskStatistic();
      else
        DisplayDirStatistic(ctx.de_ptr);

      BuildFileEntryList(ctx.dir_entry);

      if( file_entry_list.size() == 0 ) ctx.unput_char = ESC;

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

      DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);
      ctx.maybe_change_x_step = true;
          }
          break;

      case 'N' & 0x1F :
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      beep();
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
          beep();
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
        beep();
        break;
            }

      ctx.term = InputChoise("Confirm overwrite existing files (Y/N) ? ", "YN\033");
                        if( ctx.term == ESC )
            {
        beep();
        break;
      }

      MoveWalkContext move_ctx;
      move_ctx.dest_dir_entry = ctx.dest_dir_entry;
      move_ctx.to_file = ctx.to_file;
      move_ctx.to_path = ctx.to_path;
      move_ctx.confirm = (ctx.term == 'Y');

      WalkTaggedFiles(ctx.dir_entry->start_file,
           ctx.dir_entry->cursor_pos,
           MoveTaggedFiles,
           &move_ctx
);

      BuildFileEntryList(ctx.dir_entry);

      if( file_entry_list.size() == 0 ) ctx.unput_char = ESC;

      ctx.dir_entry->start_file = 0;
      ctx.dir_entry->cursor_pos = 0;

      DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);
      ctx.maybe_change_x_step = true;
          }
          break;

      case 'D' :
      case 'd' :      if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
          {
      beep();
      break;
          }

          ctx.term = InputChoise("Delete this file (Y/N) ? ",
            "YN\033"
);

          ctx.need_dsp_help = true;

          if( ctx.term != 'Y' ) break;

          ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          if( !DeleteFile(ctx.fe_ptr) )
          {
            /* File wurde geloescht */
      /*----------------------*/

      if( ctx.dir_entry->global_flag )
        DisplayDiskStatistic();
      else
        DisplayDirStatistic(ctx.de_ptr);

      DisplayAvailBytes();

                        RemoveFileEntry(ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos);

      if( file_entry_list.size() == 0 ) ctx.unput_char = ESC;

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

      DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);
      ctx.maybe_change_x_step = true;
          }
                      break;

      case 'D' & 0x1F :
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      beep();
          }
          else
          {
            ctx.need_dsp_help = true;
      DeleteTaggedFiles(max_disp_files);
      if( file_entry_list.size() == 0 ) ctx.unput_char = ESC;
      ctx.dir_entry->start_file = 0;
      ctx.dir_entry->cursor_pos = 0;
                        DisplayAvailBytes();
      DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);
      ctx.maybe_change_x_step = true;
          }
          break;

      case 'R':
      case 'r':       if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
          {
      beep();
      break;
          }

          ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();

          if (const auto renamed = GetRenameParameter(&ctx.fe_ptr->name))
          {
      if( !RenameFile(ctx.fe_ptr, *renamed, &ctx.new_fe_ptr) )
            {
        /* Rename OK */
        /*-----------*/

        /* Maybe structure has changed... */
        /*--------------------------------*/

        BuildFileEntryList(ctx.de_ptr);

        DisplayFiles(ctx.de_ptr,
                ctx.dir_entry->start_file,
                ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.start_x
);
        ctx.maybe_change_x_step = true;
                        }
          }
          ctx.need_dsp_help = true;
          break;

      case 'R' & 0x1F :
          if(( mode != Mode::DISK_MODE && mode != Mode::USER_MODE) || !IsMatchingTaggedFiles() )
          {
      beep();
          }
          else
          {
            ctx.need_dsp_help = true;

      const auto tagged_rename = GetRenameParameter(nullptr);
      if (!tagged_rename)
      {
        beep();
        break;
      }

      RenameWalkContext rename_ctx;
      rename_ctx.new_name = *tagged_rename;
      rename_ctx.confirm = false;

      WalkTaggedFiles(ctx.dir_entry->start_file,
           ctx.dir_entry->cursor_pos,
           RenameTaggedFiles,
           &rename_ctx
);

      BuildFileEntryList(ctx.dir_entry);

      if( file_entry_list.size() == 0 ) ctx.unput_char = ESC;

      DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);

      ctx.maybe_change_x_step = true;
          }
          break;

      case 'S':
      case 's':       GetKindOfSort();

          ctx.dir_entry->start_file = 0;
          ctx.dir_entry->cursor_pos = 0;

          SortFileEntryList();

          DisplayFiles(ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
);
          ctx.need_dsp_help = true;
          break;

      case 'F':
      case 'f':       if(ReadFileSpec()) {

            ctx.dir_entry->start_file = 0;
            ctx.dir_entry->cursor_pos = 0;

            BuildFileEntryList(ctx.dir_entry);

            DisplayFileSpec();
            DisplayFiles(ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
);

            if( ctx.dir_entry->global_flag )
              DisplayDiskStatistic();
            else
              DisplayDirStatistic(ctx.dir_entry);

                        if( file_entry_list.size() == 0 ) ctx.unput_char = ESC;
            ctx.maybe_change_x_step = true;
                }
          ctx.need_dsp_help = true;
          break;

#ifndef VI_KEYS
      case 'l':
#endif /* VI_KEYS */
      case 'L':
        ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
        if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
        {
          auto new_login_path = GetFileNamePath(ctx.fe_ptr).string();

          if (const auto login_path = GetNewLoginPath(new_login_path))
          {
            ctx.dir_entry->login_flag = true;
            LoginDisk(*login_path);
            ctx.unput_char = LOGIN_ESC;
          }
          ctx.need_dsp_help = true;
        } else {
          beep();
        }
        break;

      case LF:
      case CR:        if( ctx.dir_entry->big_window ) break;
          ctx.dir_entry->big_window = true;
          ch = '\0';
          SwitchToBigFileWindow();
                      GetMaxYX(file_window, &window_height, &window_width);

          x_step =  (max_column > 1) ? window_height : 1;
                      max_disp_files = window_height * max_column;

          DisplayFiles(ctx.dir_entry,
            ctx.dir_entry->start_file,
            ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
            ctx.start_x
);
          break;

      case 'P' :
      case 'p' :      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();
          Pipe(ctx.de_ptr, ctx.fe_ptr);
          ctx.need_dsp_help = true;
          break;

      case 'P' & 0x1F:
          ctx.de_ptr = ctx.dir_entry;

          if (!IsMatchingTaggedFiles())
          {
            beep();
          }
          else if( mode != Mode::DISK_MODE && mode != Mode::USER_MODE )
          {
            Message("ctx.i am sorry*^P not supported in Archive-mode");
          } else {
            std::string filepath;

            ctx.need_dsp_help = true;

            if (const auto pipe_command = GetPipeCommand(filepath))
            {
              filepath = *pipe_command;
            } else {
              beep();
              break;
            }

            PipeWalkContext pipe_ctx;
            if (!(pipe_ctx.pipe_file = popen(filepath.c_str(), "w")))
            {
              FormatMessage("execution of command*{}*failed", filepath);
              break;
            }


            WalkTaggedFiles(
              ctx.dir_entry->start_file,
              ctx.dir_entry->cursor_pos,
              PipeTaggedFiles,
              &pipe_ctx
            );

            clearok(stdscr, true);

            if (pclose(pipe_ctx.pipe_file) )
            {
              Warning("pclose() failed");
            }

            GetAvailBytes(&statistic.disk_space);
            DisplayAvailBytes();

            DisplayFiles(
              ctx.dir_entry,
              ctx.dir_entry->start_file,
              ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
              ctx.start_x
            );
          }
          break;

      case 'X':
      case 'x' :      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
          ctx.de_ptr = ctx.fe_ptr->Dir().get();
          Execute(ctx.de_ptr, ctx.fe_ptr);
          ctx.need_dsp_help = true;
          break;

      case 'S' & 0x1F :
          if (!IsMatchingTaggedFiles())
          {
            beep();
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
              refresh();
              endwin();
              SuspendClock();

              ExecuteWalkContext execute_ctx;
              execute_ctx.command = command_line;
              SilentTagWalkTaggedFiles(ExecuteCommand, &execute_ctx);

              HitReturnToContinue();
              refresh();
              InitClock();
              RefreshWindow(file_window);

              DisplayFiles(
                ctx.dir_entry,
                ctx.dir_entry->start_file,
                ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.start_x
              );
            }
          }
          break;

      case 'X' & 0x1F:
          if( !IsMatchingTaggedFiles() )
          {
            beep();
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
              refresh();
              endwin();
              ExecuteWalkContext execute_ctx;
              execute_ctx.command = command_line;
              SilentWalkTaggedFiles(ExecuteCommand, &execute_ctx);
              HitReturnToContinue();
              refresh();
              InitClock();

              DisplayFiles(
                ctx.dir_entry,
                ctx.dir_entry->start_file,
                ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos,
                ctx.start_x
              );
            }
          }
          break;

      case 'Q' & 0x1F:
                      ctx.need_dsp_help = true;
                      ctx.fe_ptr = file_entry_list[ctx.dir_entry->start_file + ctx.dir_entry->cursor_pos].get();
                      ctx.de_ptr = ctx.fe_ptr->Dir().get();
                      QuitTo(ctx.de_ptr);
                      break;

      case 'Q':
      case 'q':       ctx.need_dsp_help = true;
                      Quit();
          break;

      case 'L' & 0x1F:
          clearok(stdscr, true);
          break;

      case '\033':    break;

      case LOGIN_ESC :
          break;

      case KEY_F(12):
                ListJump(ctx.dir_entry, "");
          ctx.need_dsp_help = true;
          break;

      default:        beep();
          break;
  }
}

static bool ShouldContinueFileWindow(int ch)
{
  return ch != CR && ch != ESC && ch != LOGIN_ESC;
}

static void FinalizeFileWindow(FileWindowContext& ctx, int ch)
{
  if (ctx.dir_entry->big_window)
  {
    SwitchToSmallFileWindow();
  }

  if (ch != LOGIN_ESC)
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

  int ch = 0;
  do
  {
    UpdateFileWindowLayout(ctx);

    if (ctx.need_dsp_help)
    {
      ctx.need_dsp_help = false;
      DisplayFileHelp();
    }

    ch = ReadFileWindowKey(ctx);
    HandleFileWindowResize(ctx);
    RemapFileModeNavigationKeys(ch);
    ResetFileWindowHorizontalScroll(ch, ctx);
    ApplyFileUserMode(ch, ctx);
    ProcessFileWindowKey(ch, ctx);
  } while (ShouldContinueFileWindow(ch));

  FinalizeFileWindow(ctx, ch);
  return ch;
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






static int DeleteTaggedFiles(int max_disp_files)
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
  int       result = 0;

  term = InputChoise("Confirm delete each file (Y/N) ? ", "YN\033");

  if( term == ESC ) return( -1 );

  if( term == 'Y' ) confirm = true;
  else confirm = false;

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead(0);

  for( i=0; i < (int)file_entry_list.size() && result == 0; )
  {
    deleted = false;

    fe_ptr = file_entry_list[i].get();
    de_ptr = fe_ptr->Dir().get();

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      start_file = std::max(0, i - max_disp_files + 1);
      cursor_pos = i - start_file;

      DisplayFiles(de_ptr,
        start_file,
        start_file + cursor_pos,
        start_x
);

      if( fe_ptr->Dir()->global_flag )
        DisplayGlobalFileParameter(fe_ptr);
      else
        DisplayFileParameter(fe_ptr);

      RefreshWindow(file_window);
      doupdate();

      if( confirm ) term = InputChoise("Delete this file (Y/N) ? ", "YN\033");
      else term = 'Y';

      if( term == ESC )
      {
        if( baudrate() >= QUICK_BAUD_RATE ) typeahead(-1);
  result = -1;
  break;
      }

      if( term == 'Y' )
      {
        if( ( result = DeleteFile(fe_ptr) ) == 0 )
        {
    /* File wurde geloescht */
    /*----------------------*/

    deleted = true;

      if( de_ptr->global_flag )
      DisplayDiskStatistic();
    else
      DisplayDirStatistic(de_ptr);

    DisplayAvailBytes();

          RemoveFileEntry(start_file + cursor_pos);
        }
      }
    }
    if( !deleted ) i++;
  }
  if( baudrate() >= QUICK_BAUD_RATE ) typeahead(-1);

  return( result );
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

    /*  in file_window press initial char of file to jump to it */

    FileEntry * fe_ptr = nullptr;
    int i=0, j=0, n=0, start_x=0, ic=0, tmp2=0;
    const char * jumpmsg = "Press initial of file to jump to... ";

    ClearHelp();
    MvAddStr(LINES - 2, 1, jumpmsg);
    PrintOptions
    (
        stdscr,
        LINES - 2,
        COLS - 14,
        "(Escape) cancel"
    );

    ic = std::tolower(getch());

    if( !std::isprint(ic) )
    {
        beep();
        return;
    }

    n = std::strlen(str);
    std::string newStr(str);
    newStr += static_cast<char>(ic);

    /* index of current entry in list */
    tmp2 = (incremental && n == 0) ? 0 : dir_entry->start_file + dir_entry->cursor_pos;

    if( tmp2 == static_cast<int>(file_entry_list.size() - 1))
    {
        ClearHelp();
        MvAddStr(LINES - 2, 1, "Last entry!");
        beep();
        RefreshWindow(stdscr);
        RefreshWindow(file_window);
        doupdate();
        sleep(1);
        return;
    }

    for( i=tmp2; i < static_cast<int>(file_entry_list.size()); i++ )
    {
        fe_ptr = file_entry_list[i].get();
  if(!strncasecmp(newStr.c_str(), fe_ptr->name.c_str(), n+1))
          break;
    }

    if ( i == static_cast<int>(file_entry_list.size()) )
    {
        ClearHelp();
        MvAddStr(LINES - 2, 1, "No match!");
        beep();
        RefreshWindow(stdscr);
        RefreshWindow(file_window);
        doupdate();
        sleep(1);
        return;
    }

    /* position cursor on entry wanted and found */
    if( incremental && n == 0 ) {
        /* first search start on top */
        dir_entry->start_file = 0;
        dir_entry->cursor_pos = 0;
        DisplayFiles(dir_entry,
              dir_entry->start_file,
              dir_entry->start_file + dir_entry->cursor_pos,
              start_x
);
    }
    for ( j=tmp2; j < i; j++ )
        fmovedown
        (
            &dir_entry->start_file,
            &dir_entry->cursor_pos,
            &start_x,
            dir_entry
        );
    RefreshWindow(stdscr);
    RefreshWindow(file_window);
    doupdate();
    ListJump(dir_entry, (incremental) ? newStr.c_str() : "");
}



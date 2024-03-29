#include "ytree.h"

#include <functional>
#include <vector>

static bool reverse_sort;
static bool order;
static bool do_case = false;
static int  file_mode;
static int  max_column;

static int  window_height;
static int  window_width;
static int  max_disp_files;
static int  x_step;
static int  my_x_step;
static int  hide_left;
static int  hide_right;

static std::vector<FileEntry*> file_entry_list;
static unsigned      max_userview_len;
static std::size_t max_filename_len;
static std::size_t max_linkname_len;
static std::size_t global_max_filename_len;
static std::size_t global_max_linkname_len;

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
static void WalkTaggedFiles(
  int,
  int,
  const std::function<int(FileEntry*, WalkingPackage*)>&,
  WalkingPackage*
);
static bool IsMatchingTaggedFiles(void);
static void RemoveFileEntry(int entry_no);
static void ChangeFileEntry(void);
static int  DeleteTaggedFiles(int max_dispfiles);
static void SilentWalkTaggedFiles(
  const std::function<int(FileEntry*, WalkingPackage*)>&,
  WalkingPackage*
);
static void SilentTagWalkTaggedFiles(
  const std::function<int(FileEntry*, WalkingPackage*)>&,
  WalkingPackage*
);
static void RereadWindowSize(DirEntry *dir_entry);
static void ListJump( DirEntry * dir_entry, const char *str );



void SetFileMode(int new_file_mode)
{

  GetMaxYX( file_window, &window_height, &window_width );
  file_mode = new_file_mode;
  switch( file_mode )
  {
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
					                   USERVIEW);
                 if(max_userview_len)
		   max_column = window_width / (max_userview_len + 1);
		 else
		   max_column = 0;
		 break;
  }

  if( max_column == 0 )
    max_column = 1;
}



void RotateFileMode(void)
{
  switch( file_mode )
  {
    case MODE_1: SetFileMode( MODE_3 ); break;
    case MODE_2: SetFileMode( MODE_5 ); break;
    case MODE_3: SetFileMode( MODE_4 ); break;
    case MODE_4: SetFileMode( MODE_2 ); break;
    case MODE_5: SetFileMode( MODE_1 ); break;
  }
  if( (mode != DISK_MODE && mode != USER_MODE) && file_mode == MODE_4 ) {
    RotateFileMode();
  } else if(file_mode == MODE_5 && !strcmp(USERVIEW, "")) {
    RotateFileMode();
  }
}

static void ReadTaggedList(const DirEntry* dir_entry)
{
  max_filename_len = 0;
  max_linkname_len = 0;

  for (auto fe_ptr = dir_entry->file; fe_ptr; fe_ptr = fe_ptr->next)
  {
    if (fe_ptr->matching && fe_ptr->tagged)
    {
      const auto name_len = std::strlen(fe_ptr->name);

      file_entry_list.push_back(fe_ptr);
      if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
      {
	      const auto linkname_len = std::strlen(&fe_ptr->name[name_len + 1]);

	      max_linkname_len = std::max(max_linkname_len, linkname_len);
      }
      max_filename_len = std::max(max_filename_len, name_len);
    }
  }
}

static void ReadTaggedFileList(const DirEntry* dir_entry)
{
  for (auto de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next)
  {
    if (de_ptr->sub_tree)
    {
      ReadTaggedFileList(de_ptr->sub_tree);
    }
    ReadTaggedList(de_ptr);
    global_max_filename_len = std::max(global_max_filename_len, max_filename_len);
    global_max_linkname_len = std::max(global_max_linkname_len, max_linkname_len);
  }
  max_filename_len = global_max_filename_len;
  max_linkname_len = global_max_linkname_len;
}

static void BuildFileEntryList(DirEntry *dir_entry)
{
  file_entry_list.clear();
  if( !dir_entry->global_flag )  {
     /* ... for !ANSI-Systeme ... */
     /*----------------------------*/
     ReadFileList( dir_entry );
     SortFileEntryList();
     SetFileMode( file_mode ); /* recalc */
  }  else if (!dir_entry->tagged_flag)  {
    global_max_filename_len = 0;
    global_max_linkname_len = 0;
    ReadGlobalFileList( statistic.tree );
    SortFileEntryList();
    SetFileMode( file_mode ); /* recalc */
  } else  {
    global_max_filename_len = 0;
    global_max_linkname_len = 0;
    ReadTaggedFileList( statistic.tree );
    SortFileEntryList();
    SetFileMode( file_mode ); /* recalc */
  }
}

static void ReadFileList(const DirEntry* dir_entry)
{
  max_filename_len = 0;
  max_linkname_len = 0;

  for (auto fe_ptr = dir_entry->file; fe_ptr; fe_ptr = fe_ptr->next)
  {
    if (fe_ptr->matching)
    {
      const auto name_len = std::strlen(fe_ptr->name);

      file_entry_list.push_back(fe_ptr);
      if (S_ISLNK(fe_ptr->stat_struct.st_mode))
      {
	      const auto linkname_len = std::strlen(&fe_ptr->name[name_len + 1]);

	      max_linkname_len = std::max(max_linkname_len, linkname_len);
      }
      max_filename_len = std::max(max_filename_len, name_len);
    }
  }
}

static void ReadGlobalFileList(const DirEntry* dir_entry)
{
  for (auto de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next)
  {
    if (de_ptr->sub_tree)
    {
      ReadGlobalFileList(de_ptr->sub_tree);
    }
    ReadFileList(de_ptr);
    global_max_filename_len = std::max(global_max_filename_len, max_filename_len);
    global_max_linkname_len = std::max(global_max_linkname_len, max_linkname_len);
  }
  max_filename_len = global_max_filename_len;
  max_linkname_len = global_max_linkname_len;
}

static void SortFileEntryList()
{
  int aux = statistic.kind_of_sort;
  std::function<bool(const FileEntry*, const FileEntry*)> compare;

  reverse_sort = false;
  if (aux > SORT_DSC)
  {
     order = false;
     aux -= SORT_DSC;
  } else {
     order = true;
     aux -= SORT_ASC;
  }
  switch (aux)
  {
    case SORT_BY_NAME: compare = SortByName; break;
    case SORT_BY_MOD_TIME: compare = SortByModTime; break;
    case SORT_BY_CHG_TIME: compare = SortByChgTime; break;
    case SORT_BY_ACC_TIME: compare = SortByAccTime; break;
    case SORT_BY_OWNER: compare = SortByOwner; break;
    case SORT_BY_GROUP: compare = SortByGroup; break;
    case SORT_BY_SIZE: compare = SortBySize; break;
    case SORT_BY_EXTENSION: compare = SortByExtension; break;
    default: compare = SortByName; beep();
  }

  std::sort(
    file_entry_list.begin(),
    file_entry_list.end(),
    compare
  );
}

static bool SortByName(const FileEntry* e1, const FileEntry* e2)
{
  if (do_case)
     if (order)
        return std::strcmp(e1->name, e2->name) < 0;
     else
        return -std::strcmp(e1->name, e2->name) < 0;
  else
     if (order)
        return strcasecmp(e1->name, e2->name) < 0;
     else
        return -strcasecmp(e1->name, e2->name) < 0;
}

static bool SortByExtension(const FileEntry* e1, const FileEntry* e2)
{
  const auto ext1 = GetExtension(e1->name).value_or("");
  const auto ext2 = GetExtension(e2->name).value_or("");
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

void SetKindOfSort(int new_kind_of_sort)
{
  statistic.kind_of_sort = new_kind_of_sort;
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
    const auto length = std::strlen(entry->name);

    max_filename_len = std::max<unsigned int>(max_filename_len, length);
    if (S_ISLNK(entry->stat_struct.st_mode))
    {
      max_linkname_len = std::max<unsigned int>(
        max_filename_len,
        std::strlen(&entry->name[length + 1])
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
  char attributes[11];
  char modify_time[13];
  char change_time[13];
  char access_time[13];
  char format[60];
  char justify;
  char *line_ptr;
  int  n, pos_x = 0;
  FileEntry *fe_ptr;
  static char* line_buffer = nullptr;
  static int  old_cols = -1;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  int  ef_window_width;
  const char* sym_link_name = nullptr;
  char type_of_file = ' ';


  ef_window_width = window_width - 2; /* Effektive Window-Width */

  (reverse_sort) ? (justify='+') : (justify='-');

  if( old_cols != COLS )
  {
    old_cols = COLS;
    if( line_buffer ) free( line_buffer );
    line_buffer = MallocOrAbort<char>(COLS + PATH_LENGTH);
  }

  fe_ptr = file_entry_list[entry_no];

  if( fe_ptr && S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    sym_link_name = &fe_ptr->name[strlen(fe_ptr->name)+1];
  else
    sym_link_name = "";


  type_of_file = GetTypeOfFile(fe_ptr->stat_struct);

  switch( file_mode )
  {
    case MODE_1 : if( fe_ptr )
		  {
		    (void) GetAttributes( fe_ptr->stat_struct.st_mode,
		                          attributes
				        );

		    (void) CTime( fe_ptr->stat_struct.st_mtime, modify_time );



                    if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
		    {
		      (void) sprintf( format, "%%c%%c%%-%lds %%10s %%3d %%11lld %%12s -> %%-%lds",
				      max_filename_len,
				      max_linkname_len
				    );

		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      attributes,
				      fe_ptr->stat_struct.st_nlink,
                                      (long long) fe_ptr->stat_struct.st_size,
				      modify_time,
				      sym_link_name
				    );
                    }
		    else
		    {
		      (void) sprintf( format, "%%c%%c%%%c%lds %%10s %%3d %%11lld %%12s",
                                      justify,
				      max_filename_len
				    );

		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      attributes,
				      fe_ptr->stat_struct.st_nlink,
                                      (long long) fe_ptr->stat_struct.st_size,
				      modify_time
				    );
                    }
		  }
		  else
		  {
		    /* Empty Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%lds", max_filename_len + 42 );
		    (void) sprintf( line_buffer, format, "" );
		  }

		  if( max_linkname_len )
		    pos_x = x * (max_filename_len + max_linkname_len + 47);
		  else
		    pos_x = x * (max_filename_len + 43);
		  break;

    case MODE_2 : if( fe_ptr )
		  {
		    (void) GetAttributes( fe_ptr->stat_struct.st_mode,
		                          attributes
				        );

        if (const auto owner_name_ptr = GetPasswdName(fe_ptr->stat_struct.st_uid))
        {
          std::strncpy(owner, owner_name_ptr->c_str(), sizeof(owner));
        } else {
          std::snprintf(owner, sizeof(owner), "%d", fe_ptr->stat_struct.st_uid);
        }
        if (const auto group_name_ptr = GetGroupName(fe_ptr->stat_struct.st_gid))
        {
          std::strncpy(group, group_name_ptr->c_str(), sizeof(group));
        } else {
          std::snprintf(group, sizeof(group), "%d", fe_ptr->stat_struct.st_gid);
        }

                    if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
		    {
                      (void) sprintf( format, "%%c%%c%%%c%lds %%10lld %%-12s %%-12s -> %%-%lds",
                                      justify,
			              max_filename_len,
			              max_linkname_len
				      );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      (long long)fe_ptr->stat_struct.st_ino,
              owner,
              group,
				      sym_link_name
				    );
                    }
		    else
		    {
                      (void) sprintf( format, "%%c%%c%%%c%lds %%10lld %%-12s %%-12s",
                                      justify,
			              max_filename_len
				      );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      (long long)fe_ptr->stat_struct.st_ino,
              owner,
              group
				    );

                    }
	          }
		  else
		  {
		    /* Empty-Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%lds", max_filename_len + 38 );
		    (void) sprintf( line_buffer, format, "" );
		  }

		  if( max_linkname_len )
                    pos_x = x * (max_filename_len + max_linkname_len + 43);
		  else
                    pos_x = x * (max_filename_len + 39);
		  break;

    case MODE_3 : if( fe_ptr )
		  {
		    (void) sprintf( format, "%%c%%c%%%c%lds",
                                    justify,
                                    max_filename_len );

		    (void) sprintf( line_buffer, format,
				    (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				    type_of_file,
				    fe_ptr->name
				  );
                  }
		  else
		  {
		    /* Empty-Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%lds", max_filename_len + 2 );
		    (void) sprintf( line_buffer, format, "" );
		  }

		  pos_x = x * (max_filename_len + 3);
		  break;

    case MODE_4 : if( fe_ptr )
		  {
		    (void) CTime( fe_ptr->stat_struct.st_ctime, change_time );
		    (void) CTime( fe_ptr->stat_struct.st_atime, access_time );

                    if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
		    {
                      (void) sprintf( format, "%%c%%c%%%c%lds Chg: %%12s  Acc: %%12s -> %%-%lds",
                                      justify,
				      max_filename_len,
				      max_linkname_len
				    );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      change_time,
				      access_time,
				      sym_link_name
				  );
                    }
		    else
		    {
                      (void) sprintf( format, "%%c%%c%%%c%lds Chg: %%12s  Acc: %%12s",
                                      justify,
				      max_filename_len
				    );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      change_time,
				      access_time
				  );
                    }
		  }
		  else
		  {
		    /* Empty-Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%lds", max_filename_len + 39 );
		    (void) sprintf( line_buffer, format, "" );
		  }


		  if( max_linkname_len )
		    pos_x = x * (max_filename_len + max_linkname_len + 44);
		  else
		    pos_x = x * (max_filename_len + 40);
		  break;

    case MODE_5 : if( fe_ptr )
		  {
 		    BuildUserFileEntry(fe_ptr,  max_filename_len, max_linkname_len,
		        USERVIEW,
		        200, line_buffer);
		  }
		  else
		  {
		    /* Empty-Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%ds", max_userview_len );
		    (void) sprintf( line_buffer, format, "" );
		  }
		  pos_x = x * (max_userview_len + 1);
		  break;

  }

  /* display line */
  /*--------------*/

  n = StrVisualLength( line_buffer );

  if( n <= ef_window_width )
  {
    /* line fits */
    /*-----------*/

    hide_left = 0;
    hide_right = 0;
    line_ptr = line_buffer;
  }
  else
  {
    /* ... does not fit; use start_x */
    /*-------------------------------*/

    if( n > ( start_x + ef_window_width ) )
      line_ptr = &line_buffer[start_x];  /* TODO: UTF-8 */
    else
      line_ptr = &line_buffer[n - ef_window_width];  /* TODO: UTF-8 */
    hide_left = start_x;
    hide_right = n - start_x - ef_window_width;
    line_ptr[ef_window_width] = '\0';
  }

#ifdef NO_HIGHLIGHT
  line_ptr[1] = (hilight) ? '>' : ' ';
  mvwaddstr( file_window, y, pos_x + 1, line_ptr );
#else
#ifdef COLOR_SUPPORT
  if( hilight )
    WbkgdSet(file_window, COLOR_PAIR(HIFILE_COLOR)|A_BOLD);
  else
    WbkgdSet(file_window, COLOR_PAIR(FILE_COLOR));

  mvwaddstr(file_window, y, pos_x + 1, line_ptr );
  WbkgdSet(file_window, COLOR_PAIR(FILE_COLOR)|A_BOLD);

#else
#endif /* COLOR_SUPPORT */
  if( hilight ) wattrset( file_window, A_REVERSE );
  mvwaddstr( file_window, y, pos_x + 1, line_ptr );
  if( hilight ) wattrset( file_window, 0 );
#endif /* NO_HIGHLIGHT */


}





void DisplayFileWindow(DirEntry *dir_entry)
{
  GetMaxYX( file_window, &window_height, &window_width );
  BuildFileEntryList( dir_entry );
  DisplayFiles( dir_entry, dir_entry->start_file,
                dir_entry->start_file + dir_entry->cursor_pos, 0);
}




static void DisplayFiles(DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x)
{
  int  x, y, p_x, p_y, j;

  werase( file_window );

  if( file_entry_list.size() == 0 )
  {
    mvwaddstr( file_window,
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
	  PrintFileEntry( j, y, x, false, start_x);
	}
      }
      j++;
    }
  }

  if( p_x >= 0 )
    PrintFileEntry( hilight_no, p_y, p_x, true, start_x);

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
          PrintFileEntry( *start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          false,
                          *start_x
                          );
          (*cursor_pos)++;
          PrintFileEntry( *start_file + *cursor_pos,
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
          DisplayFiles( dir_entry,
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
         PrintFileEntry( *start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         false,
                         *start_x
                         );
         (*cursor_pos)--;
         PrintFileEntry( *start_file + *cursor_pos,
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
         DisplayFiles( dir_entry,
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
      PrintFileEntry( *start_file + *cursor_pos,
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
          PrintFileEntry( *start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          false,
                          *start_x
                          );
          *cursor_pos += my_x_step;
          PrintFileEntry( *start_file + *cursor_pos,
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
          DisplayFiles( dir_entry,
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
            PrintFileEntry( *start_file + *cursor_pos,
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
             PrintFileEntry( *start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             false,
                             *start_x
                             );
             *cursor_pos -= my_x_step;
             PrintFileEntry( *start_file + *cursor_pos,
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
             DisplayFiles( dir_entry,
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
         PrintFileEntry( *start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         false,
                         *start_x
                         );
         if( *start_file + max_disp_files <= (int)file_entry_list.size() - 1 )
            *cursor_pos = max_disp_files - 1;
         else
            *cursor_pos = file_entry_list.size() - *start_file - 1;
         PrintFileEntry( *start_file + *cursor_pos,
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
        DisplayFiles( dir_entry,
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
             PrintFileEntry( *start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             false,
                             *start_x
                             );
             *cursor_pos = 0;
             PrintFileEntry( *start_file + *cursor_pos,
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
            DisplayFiles( dir_entry,
                          *start_file,
                          *start_file + *cursor_pos,
                          *start_x
                          );
        }
     }
     return;
}




int HandleFileWindow(DirEntry *dir_entry)
{
  FileEntry *fe_ptr;
  FileEntry *new_fe_ptr;
  DirEntry  *de_ptr = NULL;
  DirEntry  *dest_dir_entry;
  WalkingPackage walking_package;
  int ch;
  int tmp2;
  int unput_char;
  int list_pos;
  long long file_size;
  int i;
  int owner_id;
  int group_id;
  int start_x = 0;
  char filepath[PATH_LENGTH +1];
  char modus[11];
  bool path_copy;
  int  term;
  int  mask;
  static char to_dir[PATH_LENGTH+1];
  static char to_path[PATH_LENGTH+1];
  static char to_file[PATH_LENGTH+1];
  bool need_dsp_help;
  bool maybe_change_x_step;
  char new_name[PATH_LENGTH+1];
  int  dir_window_width, dir_window_height;


  unput_char = '\0';
  fe_ptr = NULL;


  /* Cursor-Positionsmerker zuruecksetzen */
  /*--------------------------------------*/

  need_dsp_help = true;
  maybe_change_x_step = true;

  BuildFileEntryList( dir_entry );

  if( dir_entry->global_flag || dir_entry->big_window || dir_entry->tagged_flag)
  {
    SwitchToBigFileWindow();
    GetMaxYX( file_window, &window_height, &window_width );
    DisplayDiskStatistic();
  }
  else
  {
    GetMaxYX( file_window, &window_height, &window_width );
    DisplayDirStatistic( dir_entry );
  }

  DisplayFiles( dir_entry,
		dir_entry->start_file,
		dir_entry->start_file + dir_entry->cursor_pos,
		start_x
	      );

  do
  {
    if( maybe_change_x_step )
    {
      maybe_change_x_step = false;

      x_step =  (max_column > 1) ? window_height : 1;
      max_disp_files = window_height * max_column;
    }

    if( need_dsp_help )
    {
      need_dsp_help = false;
      DisplayFileHelp();
    }

    if( unput_char )
    {
      ch = unput_char;
      unput_char = '\0';
    }
    else
    {
      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];

      if( dir_entry->global_flag )
        DisplayGlobalFileParameter( fe_ptr );
      else
        DisplayFileParameter( fe_ptr );

      RefreshWindow( dir_window ); /* needed: ncurses-bug ? */
      RefreshWindow( file_window );
      doupdate();
      ch = (resize_request) ? -1 : Getch();
      if( ch == LF ) ch = CR;
    }

#ifdef VI_KEYS

    ch = ViKey( ch );

#endif /* VI_KEYS */

   if(resize_request) {
     ReCreateWindows();
     RereadWindowSize(dir_entry);
     DisplayMenu();

     GetMaxYX(dir_window, &dir_window_height, &dir_window_width);
     while(statistic.cursor_pos >= dir_window_height) {
       statistic.cursor_pos--;
       statistic.disp_begin_pos++;
     }
     if(dir_entry->global_flag || dir_entry->big_window || dir_entry->tagged_flag) {

       /* big window active */

       SwitchToBigFileWindow();
       DisplayFileWindow(dir_entry);

       if(dir_entry->global_flag) {
	 DisplayDiskStatistic();
	 DisplayGlobalFileParameter(fe_ptr);
       } else {
	 DisplayFileWindow(dir_entry);
	 DisplayDirStatistic(dir_entry);
	 DisplayFileParameter(fe_ptr);
       }
     } else {

       /* small window active */

       SwitchToSmallFileWindow();
       DisplayTree( dir_window, statistic.disp_begin_pos,
		  statistic.disp_begin_pos + statistic.cursor_pos
		);
       DisplayFileWindow(dir_entry);
       DisplayDirStatistic(dir_entry);
       DisplayFileParameter(fe_ptr);
     }
     need_dsp_help = true;
     DisplayAvailBytes();
     DisplayFileSpec();
     DisplayDiskName();
     resize_request = false;
   }

   if( file_mode == MODE_1 )
   {
      if( ch == '\t' ) ch = KEY_DOWN;
      else if( ch == KEY_BTAB ) ch = KEY_UP;
   }

   if( x_step == 1 && ( ch == KEY_RIGHT || ch == KEY_LEFT ) )
   {
      /* start_x nicht zuruecksetzen */
      /*-----------------------------*/

      ; /* do nothing */
   }
   else
   {
      /* bei 0 beginnen */
      /*----------------*/

      if( start_x )
      {
	start_x = 0;

	PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
	 	        dir_entry->cursor_pos % window_height,
		        dir_entry->cursor_pos / window_height,
		        true ,
		        start_x
	              );
     }
   }

   if (mode == USER_MODE) { /* FileUserMode returns (possibly remapped) ch, or -1 if it handles ch */
      ch = FileUserMode(file_entry_list[dir_entry->start_file + dir_entry->cursor_pos], ch);
   }

   switch( ch )
   {

#ifdef KEY_RESIZE

      case KEY_RESIZE: resize_request = true;
                       break;
#endif

      case -1:         break;

      case ' ' :   /*   break;  Quick-Key */

      case KEY_DOWN :  fmovedown(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case KEY_UP   : fmoveup(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case KEY_RIGHT: fmoveright(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case KEY_LEFT : fmoveleft(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case KEY_NPAGE: fmovenpage(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case KEY_PPAGE: fmoveppage(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case KEY_END  : if( dir_entry->start_file + dir_entry->cursor_pos + 1 >= (int)file_entry_list.size() )
		      {
			/* Letzte Position erreicht */
			/*--------------------------*/

			beep();
		      }
		      else
		      {
			if( (int)file_entry_list.size() < max_disp_files )
		        {
			  dir_entry->start_file = 0;
			  dir_entry->cursor_pos = file_entry_list.size() - 1;
		        }
		        else
	                {
                          dir_entry->start_file = file_entry_list.size() - max_disp_files;
			  dir_entry->cursor_pos = file_entry_list.size() - dir_entry->start_file - 1;
		        }

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
				    );
		      }
		      break;

      case KEY_HOME : if( dir_entry->start_file + dir_entry->cursor_pos <= 0 )
		      {
			/* erste Position erreicht */
			/*-------------------------*/

			beep();
		      }
		      else
		      {
                        dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
				    );

		      }
		      break;

      case 'A' :
      case 'a' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];

	              need_dsp_help = true;

		      if( !ChangeFileModus( fe_ptr ) )
		      {
			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        true,
					start_x
			              );
		      }
		      break;

      case 'A' & 0x1F :
		      if( (mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
			need_dsp_help = true;

		   	mask = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

			(void) GetAttributes( mask, modus );

		        if( GetNewFileModus( LINES - 2, 1, modus, "\r\033" ) == CR )
			{
			  (void) strcpy( walking_package.function_data.change_modus.new_modus,
					 modus
				       );
                          WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   SetFileModus,
					   &walking_package
					 );

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}
			else
			{
			  beep();
			}
		      }
		      break;

      case 'O' :
      case 'o' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];

		      need_dsp_help = true;

		      if( !ChangeFileOwner( fe_ptr ) )
		      {
			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        true ,
					start_x
			              );
		      }
		      break;

      case 'O' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
			need_dsp_help = true;
		        if( ( owner_id = GetNewOwner( -1 ) ) >= 0 )
			{
			  walking_package.function_data.change_owner.new_owner_id = owner_id;
                          WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   SetFileOwner,
					   &walking_package
					 );

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}
		      }
		      break;

      case 'G' :
      case 'g' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];

		      need_dsp_help = true;

		      if( !ChangeFileGroup( fe_ptr ) )
		      {
			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        true,
					start_x
			              );
		      }
		      break;

      case 'G' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
			need_dsp_help = true;

		        if( ( group_id = GetNewGroup( -1 ) ) >= 0 )
			{
			  walking_package.function_data.change_group.new_group_id = group_id;
                          WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   SetFileGroup,
					   &walking_package
					 );

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}
		      }
		      break;

      case 'T' :
      case 't' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		      de_ptr = fe_ptr->dir_entry;

		      if( !fe_ptr->tagged )
		      {
                        fe_ptr->tagged = true;

			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        true,
					start_x
			              );
	       	        de_ptr->tagged_files++;
		        de_ptr->tagged_bytes += fe_ptr->stat_struct.st_size;
	       	        statistic.disk_tagged_files++;
		        statistic.disk_tagged_bytes += fe_ptr->stat_struct.st_size;
			if( dir_entry->global_flag )
			  DisplayDiskTagged();
			else
			  DisplayDirTagged( de_ptr );
		      }
		      unput_char = KEY_DOWN;

                      break;
      case 'U' :
      case 'u' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		      de_ptr = fe_ptr->dir_entry;
                      if( fe_ptr->tagged )
		      {
			fe_ptr->tagged = false;

			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        true,
					start_x
			              );

			de_ptr->tagged_files--;
			de_ptr->tagged_bytes -= fe_ptr->stat_struct.st_size;
			statistic.disk_tagged_files--;
			statistic.disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
			if( dir_entry->global_flag )
			  DisplayDiskTagged();
			else
			  DisplayDirTagged( de_ptr );
		      }

		      unput_char = KEY_DOWN;

		      break;

      case 'F' & 0x1F :
		      list_pos = dir_entry->start_file + dir_entry->cursor_pos;

		      RotateFileMode();

                      x_step =  (max_column > 1) ? window_height : 1;
                      max_disp_files = window_height * max_column;

		      if( dir_entry->cursor_pos >= max_disp_files )
		      {
			/* Cursor muss neu positioniert werden */
			/*-------------------------------------*/

                        dir_entry->cursor_pos = max_disp_files - 1;
		      }

		      dir_entry->start_file = list_pos - dir_entry->cursor_pos;
		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;

      case 'T' & 0x1F :
                      for(i=0; i < (int)file_entry_list.size(); i++)
                      {
			fe_ptr = file_entry_list[i];
			de_ptr = fe_ptr->dir_entry;

			if( !fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = true;
			  de_ptr->tagged_files++;
			  de_ptr->tagged_bytes += file_size;
			  statistic.disk_tagged_files++;
			  statistic.disk_tagged_bytes += file_size;
		        }
		      }

		      if( dir_entry->global_flag )
		        DisplayDiskTagged();
		      else
		        DisplayDirTagged( dir_entry );

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;


      case 'U' & 0x1F :
                      for(i=0; i < (int)file_entry_list.size(); i++)
                      {
			fe_ptr = file_entry_list[i];
			de_ptr = fe_ptr->dir_entry;

			if( fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = false;
			  de_ptr->tagged_files--;
			  de_ptr->tagged_bytes -= file_size;
			  statistic.disk_tagged_files--;
			  statistic.disk_tagged_bytes -= file_size;
		        }
		      }

		      if( dir_entry->global_flag )
		        DisplayDiskTagged();
		      else
		        DisplayDirTagged( dir_entry );

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;



      case ';':
      case 't' | 0x80 :
                      for(i=dir_entry->start_file + dir_entry->cursor_pos; i < (int)file_entry_list.size(); i++)
                      {
			fe_ptr = file_entry_list[i];
			de_ptr = fe_ptr->dir_entry;

			if( !fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = true;
			  de_ptr->tagged_files++;
			  de_ptr->tagged_bytes += file_size;
			  statistic.disk_tagged_files++;
			  statistic.disk_tagged_bytes += file_size;
		        }
		      }

		      if( dir_entry->global_flag )
		        DisplayDiskTagged();
		      else
		        DisplayDirTagged( dir_entry );

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;


      case ':':
      case 'u' | 0x80 :
                      for(i=dir_entry->start_file + dir_entry->cursor_pos; i < (int)file_entry_list.size(); i++)
                      {
			fe_ptr = file_entry_list[i];
			de_ptr = fe_ptr->dir_entry;

			if( fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = false;
			  de_ptr->tagged_files--;
			  de_ptr->tagged_bytes -= file_size;
			  statistic.disk_tagged_files--;
			  statistic.disk_tagged_bytes -= file_size;
		        }
		      }

		      if( dir_entry->global_flag )
		        DisplayDiskTagged();
		      else
		        DisplayDirTagged( dir_entry );

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;

      case 'V':
      case 'v':
        fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		    de_ptr = fe_ptr->dir_entry;
        View(dir_entry, GetRealFileNamePath(fe_ptr));
        need_dsp_help = true;
        break;

      case 'H':
      case 'h':
        fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		    de_ptr = fe_ptr->dir_entry;
        ViewHex(GetRealFileNamePath(fe_ptr));
        need_dsp_help = true;
        break;

      case 'E':
      case 'e':
        fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		    de_ptr = fe_ptr->dir_entry;
        Edit(de_ptr, GetFileNamePath(fe_ptr));
		    break;

      case 'Y' :
      case 'y' :
      case 'C' :
      case 'c' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		      de_ptr = fe_ptr->dir_entry;

		      path_copy = false;
		      if( ch == 'y' || ch == 'Y' ) path_copy = true;

		      need_dsp_help = true;

		      if( GetCopyParameter( fe_ptr->name, path_copy, to_file, to_dir ) )
                      {
			beep();
			break;
		      }

		      if( mode == DISK_MODE || mode == USER_MODE )
		      {
                        if( (tmp2 = GetDirEntry( statistic.tree,
				         de_ptr,
				         to_dir,
				         &dest_dir_entry,
				         to_path
				       )) == -3)
                        {
                        }
                        else if (tmp2 != 0)
		        {
			  /* beep(); */
			  break;
		        }

		        if( !CopyFile( &statistic,
				       fe_ptr,
				       true,
				       to_file,
				       dest_dir_entry,
				       to_path,
				       path_copy
				     ) )
		        {
			  /* File wurde kopiert */
			  /*--------------------*/

                          DisplayAvailBytes();

			  if( dest_dir_entry )
			  {
			    /* Ziel befindet sich im SUB-Tree */
			    /*--------------------------------*/

			    if( dir_entry->global_flag )
			      DisplayDiskStatistic();
			    else
			      DisplayDirStatistic( de_ptr );

			    if( dest_dir_entry == de_ptr )
			    {
			      /* Ziel ist aktuelles Verzeichnis */
			      /*--------------------------------*/

			      BuildFileEntryList( dir_entry );

			      DisplayFiles( dir_entry,
					    dir_entry->start_file,
					    dir_entry->start_file + dir_entry->cursor_pos,
					    start_x
				          );
			    }
			  }
		        }
		      }
		      else
		      {
			/* TAR_FILE_MODE */
			/*---------------*/

			dest_dir_entry = NULL;

			if( disk_statistic.tree )
			{
			  if( GetDirEntry( disk_statistic.tree,
				           de_ptr,
				           to_dir,
				           &dest_dir_entry,
				           to_path
				         ) )
		          {
			    beep();
			    break;
		          }
		        }
			else
			{
			  (void) strcpy( to_path, to_dir );
			}
		        if( !CopyFile( &disk_statistic,
				       fe_ptr,
				       true,
				       to_file,
				       dest_dir_entry,
				       to_path,
				       path_copy
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
		      de_ptr = dir_entry;

                      path_copy = false;
                      if( ch == ('Y' & 0x1F) ) path_copy = true;

		      if( !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
		        need_dsp_help = true;

			if( GetCopyParameter( NULL, path_copy, to_file, to_dir ) )
                        {
			  beep();
			  break;
		        }


			if( mode == DISK_MODE || mode == USER_MODE )
			{
                          if( GetDirEntry( statistic.tree,
					   de_ptr,
				           to_dir,
				           &dest_dir_entry,
				           to_path
				         ) )
		          {
			    beep();
			    break;
		          }

			  term = InputChoise( "Confirm overwrite existing files (Y/N) ? ", "YN\033" );
                          if( term == ESC )
		          {
			    beep();
			    break;
			  }

			  walking_package.function_data.copy.statistic_ptr  = &statistic;
			  walking_package.function_data.copy.dest_dir_entry = dest_dir_entry;
			  walking_package.function_data.copy.to_file        = to_file;
			  walking_package.function_data.copy.to_path        = to_path;
			  walking_package.function_data.copy.path_copy      = path_copy;
			  walking_package.function_data.copy.confirm = (term == 'Y') ? true : false;

			  WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   CopyTaggedFiles,
					   &walking_package
				         );

                          DisplayAvailBytes();


			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
		        }
		        else
		        {
			  /* TAR_FILE_MODE */
			  /*---------------*/

			  dest_dir_entry = NULL;

			  if( disk_statistic.tree )
			  {
                            if( GetDirEntry( disk_statistic.tree,
					     de_ptr,
				             to_dir,
				             &dest_dir_entry,
				             to_path
				           ) )
		            {
			      beep();
			      break;
		            }
	                  }
			  else
			  {
			    (void) strcpy( to_path, to_dir );
			  }

			  term = InputChoise( "Confirm overwrite existing files (Y/N) ? ", "YN\033" );
                          if( term == ESC )
		          {
			    beep();
			    break;
			  }

			  walking_package.function_data.copy.statistic_ptr  = &disk_statistic;
			  walking_package.function_data.copy.dest_dir_entry = dest_dir_entry;
			  walking_package.function_data.copy.to_file        = to_file;
			  walking_package.function_data.copy.to_path        = to_path;
			  walking_package.function_data.copy.path_copy      = path_copy;
			  walking_package.function_data.copy.confirm = (term == 'Y') ? true : false;

			  WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   CopyTaggedFiles,
					   &walking_package
				         );

                          DisplayAvailBytes();

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
		        }
		      }
		      break;

      case 'M' :
      case 'm' :      if( mode != DISK_MODE && mode != USER_MODE )
                      {
			beep();
			break;
		      }

		      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		      de_ptr = fe_ptr->dir_entry;

		      need_dsp_help = true;

		      if( GetMoveParameter( fe_ptr->name, to_file, to_dir ) )
                      {
			beep();
			break;
		      }

                      if( GetDirEntry( statistic.tree,
				       de_ptr,
				       to_dir,
				       &dest_dir_entry,
				       to_path
				     ) )
		      {
			beep();
			break;
		      }

		      if( !MoveFile( fe_ptr,
				     true,
				     to_file,
				     dest_dir_entry,
				     to_path,
				     &new_fe_ptr
				   ) )
		      {
			/* File wurde bewegt */
			/*-------------------*/

                        DisplayAvailBytes();

			if( dir_entry->global_flag )
			  DisplayDiskStatistic();
			else
			  DisplayDirStatistic( de_ptr );

			BuildFileEntryList( dir_entry );

			if( file_entry_list.size() == 0 ) unput_char = ESC;

			if( dir_entry->start_file + dir_entry->cursor_pos >= (int)file_entry_list.size() )
			{
			  if( --dir_entry->cursor_pos < 0 )
			  {
			    if( dir_entry->start_file > 0 )
			    {
			      dir_entry->start_file--;
			    }
			    dir_entry->cursor_pos = 0;
			  }
			}

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = true;
		      }
		      break;

      case 'N' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
		        need_dsp_help = true;

			if( GetMoveParameter( NULL, to_file, to_dir ) )
                        {
			  beep();
			  break;
		        }


                        if( GetDirEntry( statistic.tree,
					 de_ptr,
				         to_dir,
				         &dest_dir_entry,
				         to_path
				       ) )
		        {
			  beep();
			  break;
		        }

			term = InputChoise( "Confirm overwrite existing files (Y/N) ? ", "YN\033" );
                        if( term == ESC )
		        {
			  beep();
			  break;
			}

			walking_package.function_data.mv.dest_dir_entry = dest_dir_entry;
			walking_package.function_data.mv.to_file = to_file;
			walking_package.function_data.mv.to_path = to_path;
			walking_package.function_data.mv.confirm = (term == 'Y') ? true : false;

			WalkTaggedFiles( dir_entry->start_file,
					 dir_entry->cursor_pos,
					 MoveTaggedFiles,
					 &walking_package
				       );

			BuildFileEntryList( dir_entry );

			if( file_entry_list.size() == 0 ) unput_char = ESC;

			dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = true;
		      }
		      break;

      case 'D' :
      case 'd' :      if( mode != DISK_MODE && mode != USER_MODE )
		      {
			beep();
			break;
		      }

		      term = InputChoise( "Delete this file (Y/N) ? ",
					  "YN\033"
					);

		      need_dsp_help = true;

		      if( term != 'Y' ) break;

		      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		      de_ptr = fe_ptr->dir_entry;

		      if( !DeleteFile( fe_ptr ) )
		      {
		        /* File wurde geloescht */
			/*----------------------*/

			if( dir_entry->global_flag )
			  DisplayDiskStatistic();
			else
			  DisplayDirStatistic( de_ptr );

			DisplayAvailBytes();

                        RemoveFileEntry( dir_entry->start_file + dir_entry->cursor_pos );

			if( file_entry_list.size() == 0 ) unput_char = ESC;

			if( dir_entry->start_file + dir_entry->cursor_pos >= (int)file_entry_list.size() )
			{
			  if( --dir_entry->cursor_pos < 0 )
			  {
			    if( dir_entry->start_file > 0 )
			    {
			      dir_entry->start_file--;
			    }
			    dir_entry->cursor_pos = 0;
			  }
			}

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = true;
		      }
                      break;

      case 'D' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
		        need_dsp_help = true;
			(void) DeleteTaggedFiles( max_disp_files );
			if( file_entry_list.size() == 0 ) unput_char = ESC;
			dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;
                        DisplayAvailBytes();
			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = true;
		      }
		      break;

      case 'R':
      case 'r':       if( mode != DISK_MODE && mode != USER_MODE )
		      {
			beep();
			break;
		      }

		      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		      de_ptr = fe_ptr->dir_entry;

		      if( !GetRenameParameter( fe_ptr->name, new_name ) )
		      {
			if( !RenameFile( fe_ptr, new_name, &new_fe_ptr ) )
		        {
			  /* Rename OK */
			  /*-----------*/

			  /* Maybe structure has changed... */
			  /*--------------------------------*/

			  BuildFileEntryList( de_ptr );

			  DisplayFiles( de_ptr,
				        dir_entry->start_file,
				        dir_entry->start_file + dir_entry->cursor_pos,
				        start_x
			              );
			  maybe_change_x_step = true;
                        }
		      }
		      need_dsp_help = true;
		      break;

      case 'R' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
		        need_dsp_help = true;

			if( GetRenameParameter( NULL, new_name ) )
                        {
			  beep();
			  break;
		        }

			walking_package.function_data.rename.new_name = new_name;
			walking_package.function_data.rename.confirm  = false;

			WalkTaggedFiles( dir_entry->start_file,
					 dir_entry->cursor_pos,
					 RenameTaggedFiles,
					 &walking_package
				       );

			BuildFileEntryList( dir_entry );

			if( file_entry_list.size() == 0 ) unput_char = ESC;

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );

			maybe_change_x_step = true;
		      }
		      break;

      case 'S':
      case 's':       GetKindOfSort();

		      dir_entry->start_file = 0;
		      dir_entry->cursor_pos = 0;

		      SortFileEntryList();

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      need_dsp_help = true;
		      break;

      case 'F':
      case 'f':       if(ReadFileSpec() == 0) {

		        dir_entry->start_file = 0;
		        dir_entry->cursor_pos = 0;

		        BuildFileEntryList( dir_entry );

		        DisplayFileSpec();
		        DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );

		        if( dir_entry->global_flag )
		          DisplayDiskStatistic();
		        else
		          DisplayDirStatistic( dir_entry );

                        if( file_entry_list.size() == 0 ) unput_char = ESC;
		        maybe_change_x_step = true;
	              }
		      need_dsp_help = true;
		      break;

#ifndef VI_KEYS
      case 'l':
#endif /* VI_KEYS */
      case 'L':
        fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
        if (mode == DISK_MODE || mode == USER_MODE)
        {
          const auto path = GetFileNamePath(fe_ptr);
          char new_login_path[PATH_LENGTH + 1];

          std::strcpy(new_login_path, path.c_str());
          if (!GetNewLoginPath(new_login_path))
          {
            dir_entry->login_flag = true;
            LoginDisk(new_login_path);
            unput_char = LOGIN_ESC;
          }
          need_dsp_help = true;
        } else {
          beep();
        }
        break;

      case LF:
      case CR:        if( dir_entry->big_window ) break;
		      dir_entry->big_window = true;
		      ch = '\0';
		      SwitchToBigFileWindow();
                      GetMaxYX( file_window, &window_height, &window_width );

		      x_step =  (max_column > 1) ? window_height : 1;
                      max_disp_files = window_height * max_column;

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;

      case 'P' :
      case 'p' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		      de_ptr = fe_ptr->dir_entry;
		      (void) Pipe( de_ptr, fe_ptr );
		      need_dsp_help = true;
		      break;

      case 'P' & 0x1F :
		      de_ptr = dir_entry;

		      if( !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
		      {
			Message("i am sorry*^P not supported in Archive-mode");
		      }
		      else
		      {
		        need_dsp_help = true;

			if( GetPipeCommand( filepath ) )
                        {
			  beep();
			  break;
		        }



			if( ( walking_package.function_data.pipe_cmd.pipe_file =
			      popen( filepath, "w" ) ) == NULL )
			{
			  MessagePrintf("execution of command*%s*failed", filepath);
			  break;
			}


			WalkTaggedFiles( dir_entry->start_file,
					 dir_entry->cursor_pos,
					 PipeTaggedFiles,
					 &walking_package
				       );

		        clearok( stdscr, true );

			if( pclose( walking_package.function_data.pipe_cmd.pipe_file ) )
			{
			  Warning("pclose() failed");
			}

                        (void) GetAvailBytes( &statistic.disk_space );
                        DisplayAvailBytes();

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
		      }
		      break;

      case 'X':
      case 'x' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
		      de_ptr = fe_ptr->dir_entry;
		      (void) Execute( de_ptr, fe_ptr );
		      need_dsp_help = true;
		      break;

      case 'S' & 0x1F :
                      if( !IsMatchingTaggedFiles() )
                      {
                        beep();
                      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
		      {
			Message("Feature not available in archives.");
		      }
		      else
		      {
            auto command_line = MallocOrAbort<char>(COLS + 1);

			need_dsp_help = true;
			*command_line = '\0';
		        if( !GetSearchCommandLine( command_line ) )
			{
			  refresh();
			  endwin();
			  SuspendClock();

			  walking_package.function_data.execute.command = command_line;
                          SilentTagWalkTaggedFiles( ExecuteCommand,
					            &walking_package
					          );
			  RefreshWindow( file_window );

			  HitReturnToContinue();

			  InitClock();

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}
			free( command_line );
		      }
		      break;

      case 'X' & 0x1F:
		      if( !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
		      {
			Message("I am sorry*^X not supported in Archive-mode");
		      }
		      else
		      {
            auto command_line = MallocOrAbort<char>(COLS + 1);

			need_dsp_help = true;
			*command_line = '\0';
		        if( !GetCommandLine( command_line ) )
			{
			  refresh();
			  endwin();
			  walking_package.function_data.execute.command = command_line;
                          SilentWalkTaggedFiles( ExecuteCommand,
					         &walking_package
					       );
			  HitReturnToContinue();

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}
			free( command_line );
		      }
		      break;

      case 'Q' & 0x1F:
                      need_dsp_help = true;
                      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos];
                      de_ptr = fe_ptr->dir_entry;
                      QuitTo(de_ptr);
                      break;

      case 'Q':
      case 'q':       need_dsp_help = true;
                      Quit();
		      break;

      case 'L' & 0x1F:
		      clearok( stdscr, true );
		      break;

      case '\033':    break;

      case LOGIN_ESC :
		      break;

      case KEY_F(12):
      		      ListJump(dir_entry, "");
		      need_dsp_help = true;
		      break;

      default:        beep();
		      break;
    }
  } while( ch != CR && ch != ESC && ch != LOGIN_ESC );

  if( dir_entry->big_window )
    SwitchToSmallFileWindow();

  if(ch != LOGIN_ESC) {
    dir_entry->global_flag = false;
    dir_entry->tagged_flag = false;
    dir_entry->big_window  = false;
  }

  return( ch );
}




static void WalkTaggedFiles(
  int start_file,
  int cursor_pos,
  const std::function<int(FileEntry*, WalkingPackage*)>& fkt,
  WalkingPackage *walking_package
)
{
  FileEntry *fe_ptr;
  int       i;
  int       start_x = 0;
  int       result = 0;
  bool      maybe_change_x = false;

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( 0 );

/*  GetMaxYX( file_window, &window_height, &window_width );*/

  max_disp_files = window_height * max_column;

  for( i=0; i < (int)file_entry_list.size() && result == 0; i++ )
  {
    fe_ptr = file_entry_list[i];

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      if( maybe_change_x == false &&
	  i >= start_file && i < start_file + max_disp_files )
      {
	/* Walk ohne scroll moeglich */
	/*---------------------------*/

  	PrintFileEntry( start_file + cursor_pos,
			cursor_pos % window_height,
			cursor_pos / window_height,
			false,
		        start_x
	 	      );

        cursor_pos = i - start_file;

	PrintFileEntry( start_file + cursor_pos,
		 	cursor_pos % window_height,
			cursor_pos / window_height,
			true,
		        start_x
		      );
      }
      else
      {
	/* Scroll noetig */
	/*---------------*/

	start_file = std::max( 0, i - max_disp_files + 1 );
	cursor_pos = i - start_file;

        DisplayFiles( fe_ptr->dir_entry,
		      start_file,
		      start_file + cursor_pos,
		      start_x
	            );
	maybe_change_x = false;
      }

      if( fe_ptr->dir_entry->global_flag )
        DisplayGlobalFileParameter( fe_ptr );
      else
        DisplayFileParameter( fe_ptr );

      RefreshWindow( file_window );
      doupdate();
      result = fkt( fe_ptr, walking_package );
      if( walking_package->new_fe_ptr != fe_ptr )
      {
        file_entry_list[i] = walking_package->new_fe_ptr;
	ChangeFileEntry();
        max_disp_files = window_height * max_column;
	maybe_change_x = true;
      }
    }
  }

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );
}

/*
 ExecuteCommand (*fkt) had its retval zeroed as found.
 ^S needs that value, so it was unzeroed. forloop below
 was modified to not care about retval instead?
 global flag for stop-on-error? does anybody want it?

 --crb3 12mar04
*/

static void SilentWalkTaggedFiles(
  const std::function<int(FileEntry*, WalkingPackage*)>& fkt,
  WalkingPackage* walking_package
)
{
  FileEntry *fe_ptr;
  int       i;


  for( i=0; i < (int)file_entry_list.size(); i++ )
  {
    fe_ptr = file_entry_list[i];

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      fkt( fe_ptr, walking_package );
    }
  }
}

/*

SilentTagWalkTaggedFiles.
revision of above function to provide something like
XTG's <search> facility, using external grep.
- loops for entire filescount.
- if called program returns 1 (grep's "no-match" retcode), untags the file.
repeated calls can be used to pare down tags, each with a different
string, until only the intended target files are tagged.

ExecuteCommand must have its retval unzeroed.

--crb3 31dec03

*/

static void SilentTagWalkTaggedFiles(
  const std::function<int(FileEntry*, WalkingPackage*)>& fkt,
  WalkingPackage* walking_package
)
{
  FileEntry *fe_ptr;
  int       i;
  int       result = 0;


  for( i=0; i < (int)file_entry_list.size(); i++ )
  {
    fe_ptr = file_entry_list[i];

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      result = fkt( fe_ptr, walking_package );

      if( result == 0 ) {
      	fe_ptr->tagged = false;
      }
    }
  }
}




static bool IsMatchingTaggedFiles(void)
{
  FileEntry *fe_ptr;
  int i;

  for( i=0; i < (int)file_entry_list.size(); i++)
  {
    fe_ptr = file_entry_list[i];

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

  term = InputChoise( "Confirm delete each file (Y/N) ? ", "YN\033" );

  if( term == ESC ) return( -1 );

  if( term == 'Y' ) confirm = true;
  else confirm = false;

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( 0 );

  for( i=0; i < (int)file_entry_list.size() && result == 0; )
  {
    deleted = false;

    fe_ptr = file_entry_list[i];
    de_ptr = fe_ptr->dir_entry;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      start_file = std::max( 0, i - max_disp_files + 1 );
      cursor_pos = i - start_file;

      DisplayFiles( de_ptr,
		    start_file,
		    start_file + cursor_pos,
		    start_x
	          );

      if( fe_ptr->dir_entry->global_flag )
        DisplayGlobalFileParameter( fe_ptr );
      else
        DisplayFileParameter( fe_ptr );

      RefreshWindow( file_window );
      doupdate();

      if( confirm ) term = InputChoise( "Delete this file (Y/N) ? ", "YN\033" );
      else term = 'Y';

      if( term == ESC )
      {
        if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );
	result = -1;
	break;
      }

      if( term == 'Y' )
      {
        if( ( result = DeleteFile( fe_ptr ) ) == 0 )
        {
	  /* File wurde geloescht */
	  /*----------------------*/

	  deleted = true;

  	  if( de_ptr->global_flag )
	    DisplayDiskStatistic();
	  else
	    DisplayDirStatistic( de_ptr );

	  DisplayAvailBytes();

          RemoveFileEntry( start_file + cursor_pos );
        }
      }
    }
    if( !deleted ) i++;
  }
  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );

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




static void ListJump( DirEntry * dir_entry, const char *str )
{
   int incremental = (!strcmp(LISTJUMPSEARCH, "1")) ? 1 : 0; /* from ~/.ytree */

    /*  in file_window press initial char of file to jump to it */

    char *newStr = NULL;
    FileEntry * fe_ptr = NULL;
    int i=0, j=0, n=0, start_x=0, ic=0, tmp2=0;
    const char * jumpmsg = "Press initial of file to jump to... ";

    ClearHelp();
    MvAddStr( LINES - 2, 1, jumpmsg );
    PrintOptions
    (
        stdscr,
        LINES - 2,
        COLS - 14,
        "(Escape) cancel"
    );

    ic = tolower(getch());

    if( !isprint(ic) )
    {
        beep();
        return;
    }

    n = std::strlen(str);
    newStr = MallocOrAbort<char>(n + 2);
    std::strcpy(newStr, str);
    newStr[n] = ic;
    newStr[n + 1] = 0;

    /* index of current entry in list */
    tmp2 = (incremental && n == 0) ? 0 : dir_entry->start_file + dir_entry->cursor_pos;

    if( tmp2 == static_cast<int>(file_entry_list.size() - 1))
    {
        ClearHelp();
        MvAddStr( LINES - 2, 1, "Last entry!");
        beep();
        RefreshWindow( stdscr );
        RefreshWindow( file_window );
        doupdate();
        sleep(1);
        free(newStr);
        return;
    }

    for( i=tmp2; i < static_cast<int>(file_entry_list.size()); i++ )
    {
        fe_ptr = file_entry_list[i];
	if(!strncasecmp(newStr, fe_ptr->name, n+1))
          break;
    }

    if ( i == static_cast<int>(file_entry_list.size()) )
    {
        ClearHelp();
        MvAddStr( LINES - 2, 1, "No match!");
        beep();
        RefreshWindow( stdscr );
        RefreshWindow( file_window );
        doupdate();
        sleep(1);
        free(newStr);
        return;
    }

    /* position cursor on entry wanted and found */
    if( incremental && n == 0 ) {
      	/* first search start on top */
      	dir_entry->start_file = 0;
      	dir_entry->cursor_pos = 0;
      	DisplayFiles( dir_entry,
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
    RefreshWindow( stdscr );
    RefreshWindow( file_window );
    doupdate();
    ListJump( dir_entry, (incremental) ? newStr : "" );
    free(newStr);
}



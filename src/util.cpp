#include "ytree.h"

#include <filesystem>
#include <unordered_map>

static const std::unordered_map<std::string, CompressMethod> file_extensions =
{
  { ".TAP", CompressMethod::TAPE_DIR_NO_COMPRESS },
  { ".tap", CompressMethod::TAPE_DIR_NO_COMPRESS },
  { ".TAP.F", CompressMethod::TAPE_DIR_FREEZE_COMPRESS },
  { ".tap.F", CompressMethod::TAPE_DIR_FREEZE_COMPRESS },
  { ".TAP.Z", CompressMethod::TAPE_DIR_COMPRESS_COMPRESS },
  { ".tap.Z", CompressMethod::TAPE_DIR_COMPRESS_COMPRESS },
  { ".TAP.z", CompressMethod::TAPE_DIR_GZIP_COMPRESS },
  { ".tap.z", CompressMethod::TAPE_DIR_GZIP_COMPRESS },
  { ".tap.gz", CompressMethod::TAPE_DIR_GZIP_COMPRESS },
  { ".tap.bz2", CompressMethod::TAPE_DIR_BZIP_COMPRESS },
  { ".TAP.BZ2", CompressMethod::TAPE_DIR_BZIP_COMPRESS },
  { ".F", CompressMethod::  FREEZE_COMPRESS },
  { ".TFR", CompressMethod::FREEZE_COMPRESS },
  { ".Faa", CompressMethod::MULTIPLE_FREEZE_COMPRESS },
  { ".Z", CompressMethod::  COMPRESS_COMPRESS },
  { ".TZ", CompressMethod:: COMPRESS_COMPRESS },
  { ".TZR", CompressMethod::COMPRESS_COMPRESS },
  { ".Xaa", CompressMethod::MULTIPLE_COMPRESS_COMPRESS },
  { ".bz2", CompressMethod::BZIP_COMPRESS },
  { ".z", CompressMethod::  GZIP_COMPRESS },
  { ".gz", CompressMethod:: GZIP_COMPRESS },
  { ".tz", CompressMethod:: GZIP_COMPRESS },
  { ".tzr", CompressMethod::GZIP_COMPRESS },
  { ".tgz", CompressMethod::GZIP_COMPRESS },
  { ".TGZ", CompressMethod::GZIP_COMPRESS },
  { ".taz", CompressMethod::GZIP_COMPRESS },
  { ".TAZ", CompressMethod::GZIP_COMPRESS },
  { ".tpz", CompressMethod::GZIP_COMPRESS },
  { ".TPZ", CompressMethod::GZIP_COMPRESS },
  { ".xaa", CompressMethod::MULTIPLE_GZIP_COMPRESS },
  { ".zoo", CompressMethod::ZOO_COMPRESS },
  { ".ZOO", CompressMethod::ZOO_COMPRESS },
  { ".lzh", CompressMethod::LHA_COMPRESS },
  { ".LZH", CompressMethod::LHA_COMPRESS },
  { ".arc", CompressMethod::ARC_COMPRESS },
  { ".ARC", CompressMethod::ARC_COMPRESS },
  { ".rar", CompressMethod::RAR_COMPRESS },
  { ".RAR", CompressMethod::RAR_COMPRESS },
  { ".jar", CompressMethod::ZIP_COMPRESS },
  { ".zip", CompressMethod::ZIP_COMPRESS },
  { ".ZIP", CompressMethod::ZIP_COMPRESS },
  { ".JAR", CompressMethod::ZIP_COMPRESS },
  { ".rpm", CompressMethod::RPM_COMPRESS },
  { ".RPM", CompressMethod::RPM_COMPRESS },
  { ".spm", CompressMethod::RPM_COMPRESS },
  { ".SPM", CompressMethod::RPM_COMPRESS }
};

std::string GetPath(const DirEntry* dir_entry)
{
  std::string result;

  for (auto de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->up_tree)
  {
    const auto is_root =
      de_ptr->name.size() == 1 &&
      de_ptr->name[0] == std::filesystem::path::preferred_separator;

    if (!is_root)
    {
      result.insert(0, de_ptr->name);
    }
    if (de_ptr->up_tree)
    {
      result.insert(result.begin(), std::filesystem::path::preferred_separator);
    }
  }

  return result;
}

std::string GetFileNamePath(const FileEntry* file_entry)
{
  return (std::filesystem::path(GetPath(file_entry->dir_entry)) / file_entry->name).string();
}

std::string GetRealFileNamePath(const FileEntry* file_entry)
{
  if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
  {
    return GetFileNamePath(file_entry);
  }

  if (S_ISLNK(file_entry->stat_struct.st_mode))
  {
    if (!file_entry->symlink_target.empty() &&
        file_entry->symlink_target[0] == std::filesystem::path::preferred_separator)
    {
      return file_entry->symlink_target;
    }

    return (std::filesystem::path(GetPath(file_entry->dir_entry)) /
            file_entry->symlink_target)
      .string();
  }

  return (std::filesystem::path(GetPath(file_entry->dir_entry)) /
          file_entry->name)
    .string();
}

int GetDirEntry(DirEntry *tree,
                DirEntry *current_dir_entry,
                char *dir_path,
                DirEntry **dir_entry,
                char *to_path
	       )
{
  char dest_path[PATH_LENGTH+1];
  std::filesystem::path current_path;
  char *token, *old;
  DirEntry *de_ptr, *sde_ptr;
  int n;

  *dir_entry = nullptr;
  *to_path   = '\0';

  std::snprintf(to_path, PATH_LENGTH + 1, "%s", dir_path);

  if (const auto cwd = Getcwd())
  {
    current_path = *cwd;
  } else {
    Error("Getcwd failed");

    return -1;
  }

  if (*dir_path != std::filesystem::path::preferred_separator &&
      chdir(GetPath(current_dir_entry).c_str()))
  {
    Error("chdir() failed");

    return -1;
  }

  if( chdir( dir_path ) )
  {
#ifdef DEBUG
    FormatMessage("Invalid Path!*\"{}\"", dir_path);
#endif
    return( -3 );
  }

  if (*dir_path != std::filesystem::path::preferred_separator)
  {
    if (const auto cwd = Getcwd())
    {
      std::snprintf(dest_path, sizeof(dest_path), "%s", cwd->c_str());
    }
    std::snprintf(to_path, PATH_LENGTH + 1, "%s", dest_path);
  } else {
    std::snprintf(dest_path, sizeof(dest_path), "%s", dir_path);
  }

  if (chdir(current_path.c_str()))
  {
    Error("chdir() failed; Can't resume");

    return -1;
  }

  n = tree->name.size();
  const char preferred_separator_str[]{
    std::filesystem::path::preferred_separator, '\0'};
  const auto tree_is_root =
    tree->name.size() == 1 &&
    tree->name[0] == std::filesystem::path::preferred_separator;

  if( tree_is_root ||
      (tree->name.compare(0, n, dest_path, n) == 0 &&
        ( dest_path[n] == std::filesystem::path::preferred_separator || dest_path[n] == '\0' ) ) )
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree;
    token = Strtok_r( &dest_path[n], preferred_separator_str, &old );
    while( token )
    {
      for( sde_ptr = de_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next )
      {
        if( sde_ptr->name == token )
	{
	  /* Subtree gefunden */
	  /*------------------*/

	  de_ptr = sde_ptr;
	  break;
	}
      }
      if( sde_ptr == nullptr )
      {
#ifdef DEBUG
	FormatError("Can't find directory; token={}", token);
#endif
	return( -3 );
      }
      token = Strtok_r( nullptr, preferred_separator_str, &old );
    }
    *dir_entry = de_ptr;
  }
  return( 0 );
}




int GetFileEntry(DirEntry *de_ptr, char *file_name, FileEntry **file_entry)
{
  FileEntry *fe_ptr;

  *file_entry = nullptr;

  for( fe_ptr = de_ptr->file; fe_ptr; fe_ptr = fe_ptr->next )
  {
    if( fe_ptr->name == file_name )
    {
      /* Eintrag gefunden */
      /*------------------*/

      *file_entry = fe_ptr;
      break;
    }
  }
  return( 0 );
}





char *GetAttributes(unsigned short modus, char *buffer)
{
  char *save_buffer = buffer;

       if( S_ISREG( modus ) )  *buffer++ = '-';
  else if( S_ISDIR( modus ) )  *buffer++ = 'd';
  else if( S_ISCHR( modus ) )  *buffer++ = 'c';
  else if( S_ISBLK( modus ) )  *buffer++ = 'b';
  else if( S_ISFIFO( modus ) ) *buffer++ = 'p';
  else if( S_ISLNK( modus ) )  *buffer++ = 'l';
  else if( S_ISSOCK( modus ) ) *buffer++ = 's';  /* ??? */
  else                         *buffer++ = '?';  /* unknown */

  if( modus & S_IRUSR ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWUSR ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXUSR ) *buffer++ = 'x';
  else *buffer++ = '-';

  if( modus & S_ISUID ) *(buffer - 1) = 's';


  if( modus & S_IRGRP ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWGRP ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXGRP ) *buffer++ = 'x';
  else *buffer++ = '-';

  if( modus & S_ISGID ) *(buffer - 1) = 's';


  if( modus & S_IROTH ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWOTH ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXOTH ) *buffer++ = 'x';
  else *buffer++ = '-';

  *buffer = '\0';

  return( save_buffer );
}



char *CTime(time_t f_time, char *buffer)
{
  const auto now = std::time(nullptr);
  char   *cptr;

  if (now == -1)
  {
    Error("time() failed");
    std::exit(EXIT_FAILURE);
  }

  cptr = ctime( &f_time );
  (void) strncpy( buffer, cptr+4, 12 );
  buffer[12] = '\0';

  if( (now - f_time) > 31536000L )
  {
    /* Differenz groesser als 1 Jahr */
    /*-------------------------------*/

    (void) strncpy( &buffer[7], cptr + 19, 5 );

  }

  return( buffer );
}

void PrintSpecialString(
  WINDOW* win,
  int y,
  int x,
  const std::string& str,
  int color
)
{
  if (x < 0 || y < 0)
  {
    return; // Screen too small.
  }

  wmove(win, y, x);

  for (const auto& c : str)
  {
    int result;

    if (!std::iscntrl(c) || !std::isspace(c) || c == ' ')
    {
      switch (c)
      {
        case '1': result = ACS_ULCORNER; break;
        case '2': result = ACS_URCORNER; break;
        case '3': result = ACS_LLCORNER; break;
        case '4': result = ACS_LRCORNER; break;
        case '5': result = ACS_TTEE;     break;
        case '6': result = ACS_LTEE;     break;
        case '7': result = ACS_RTEE;     break;
        case '8': result = ACS_BTEE;     break;
        case '9': result = ACS_LARROW;   break;
        case '|': result = ACS_VLINE;    break;
        case '-': result = ACS_HLINE;    break;
        default:  result = PRINT(c);
      }
    } else {
      result = ACS_BLOCK;
    }
#if defined(COLOR_SUPPORT)
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#endif
    waddch(win, result);
#if defined(COLOR_SUPPORT)
    wattrset(win, 0);
#endif
  }
}

void Print(WINDOW* win, int y, int x, const std::string& str, int color)
{
  if (x < 0 || y < 0)
  {
    return; // Screen too small.
  }

  wmove(win, y, x);

  for (const auto& c : str)
  {
#if defined(COLOR_SUPPORT)
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#endif
    waddch(win, PRINT(c));
#if defined(COLOR_SUPPORT)
    wattrset(win, 0);
#endif
  }
}

void PrintOptions(WINDOW *win, int y, int x, const char *str)
{
  int ch;
  int color, hi_color, lo_color;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }

#ifdef COLOR_SUPPORT
     lo_color = MENU_COLOR;
     hi_color = HIMENUS_COLOR;
#else
     lo_color = A_NORMAL;
     hi_color = A_BOLD;
#endif

  color = lo_color;

  for( ; *str; str++ )
  {
    ch = (int) *str;

    switch( *str ) {
        case '(': color = hi_color;  continue;
	case ')': color = lo_color;  continue;

#ifdef COLOR_SUPPORT
	case ']': color = lo_color;  continue;
	case '[': color = hi_color;  continue;
#else
	case ']':
	case '[': /* ignore */ continue;
#endif

        case '1': ch = ACS_ULCORNER; break;
        case '2': ch = ACS_URCORNER; break;
        case '3': ch = ACS_LLCORNER; break;
        case '4': ch = ACS_LRCORNER; break;
        case '5': ch = ACS_TTEE;     break;
        case '6': ch = ACS_LTEE;     break;
        case '7': ch = ACS_RTEE;     break;
        case '8': ch = ACS_BTEE;     break;
        case '9': ch = ACS_LARROW;   break;
        case '|': ch = ACS_VLINE;    break;
        case '-': ch = ACS_HLINE;    break;
        default:  ch = PRINT(*str);
     }

#ifdef COLOR_SUPPORT
    wattrset( win, COLOR_PAIR(color) | A_BOLD);
#else
    wattrset( win, color);
#endif
     mvwaddch( win, y, x++, ch );
     wattrset( win, 0 );
   }
}


void PrintMenuOptions(WINDOW *win,int y, int x, char *str, int ncolor, int hcolor)
{
  int ch;
  int color, hi_color, lo_color;
  std::string sbuf;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }

#ifdef COLOR_SUPPORT
     lo_color = MENU_COLOR;
     hi_color = HIMENUS_COLOR;
#else
     lo_color = A_NORMAL;
     hi_color = A_REVERSE;
#endif

  color = lo_color;
  wmove(win, y, x);

  for( ; *str; str++ )
  {
    ch = (int) *str;

    switch( ch ) {
        case '(': color = hi_color;
#ifdef COLOR_SUPPORT
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
                  WAttrAddStr( win, color, sbuf);
#endif
		  sbuf.clear();
	          continue;

	case ')': color = lo_color;
#ifdef COLOR_SUPPORT
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
                  WAttrAddStr( win, color, sbuf);
#endif
		  sbuf.clear();
	          continue;

#ifdef COLOR_SUPPORT
	case ']': color = lo_color;
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
		  sbuf.clear();
	          continue;
	case '[': color = hi_color;
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
		  sbuf.clear();
	          continue;
#else
	case ']':
	case '[': /* ignore */ continue;
#endif
        default : sbuf += static_cast<char>(PRINT(*str));
    }
  }

#ifdef COLOR_SUPPORT
  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
  WAttrAddStr( win, color, sbuf);
#endif
}


/*****************************************************************************
 *                              FormFilename                                 *
 *****************************************************************************/

char *FormFilename(char *dest, char *src, unsigned int max_len)
{
  int i;
  int begin;
  unsigned int l;

  l = strlen(src);
  begin = 0;

  if( l <= max_len )
  {
    *std::format_to(dest, "{}", src) = '\0';
    return dest;
  }

  for(i=0; i < (int) max_len - 4; i++)
    if( src[l - i] == std::filesystem::path::preferred_separator || src[l - i] == '\\' )
      begin = l - i;
  *std::format_to(dest, "/...{}", &src[begin]) = '\0';
  return dest;
}


/*****************************************************************************
 *                              CutFilename                                  *
 *****************************************************************************/

char *CutFilename(char *dest, const char *src, unsigned int max_len)
{
  unsigned int l;

  l = StrVisualLength(src);

  if( l <= max_len )
  {
    *std::format_to(dest, "{}", src) = '\0';
    return dest;
  }

  const auto tmp = StrLeft(src, max_len - 3);
  *std::format_to(dest, "{}...", tmp) = '\0';
  return dest;
}

/*****************************************************************************
 *                              CutPathname                                  *
 *****************************************************************************/
char* CutPathname(char* dest, const std::string& src, std::size_t max_len)
{
  const auto l = src.length();

  if (l <= max_len)
  {
    *std::format_to(dest, "{}", src) = '\0';
    return dest;
  }
  *std::format_to(dest, "...{}", src.substr(l - max_len + 3)) = '\0';

  return dest;
}

/*****************************************************************************
 *                                  Fnsplit                                  *
 *****************************************************************************/

/* Aufsplitten des Dateinamens in die einzelnen Komponenten */

void Fnsplit(char *path, char *dir, char *name)
{
  int  i;
  char *name_begin;
  char *trunc_name;

  while( *path == ' ' || *path == '\t' ) path++;

  while( strchr(path, std::filesystem::path::preferred_separator ) || strchr(path, '\\') )
    *(dir++) = *(path++);

  *dir = '\0';

  name_begin = path;
  trunc_name = name;

  for(i=0; i < PATH_LENGTH && *path; i++ )
    *(name++) = *(path++);

  *name = '\0';

  if (i == PATH_LENGTH && *path)
  {
    FormatWarning("filename too long:*{}*truncating to*{}", name_begin, trunc_name);
  }
}



int BuildFilename( char *in_filename,
		   char *pattern,
		   char *out_filename
		 )
{
  char *cptr;
  int  result = 0;


  for( ; *pattern; pattern++ )
  {
    if( *pattern == '*' )
    {
      cptr = in_filename;
      for( ; (*out_filename = *cptr); out_filename++, cptr++ );
    }
    else
    {
      *out_filename++ = *pattern;
    }
  }

  *out_filename = '\0';

  return( result );
}

std::optional<CompressMethod> GetFileMethod(const std::string& filename)
{
  const auto length = filename.length();

  for (const auto& entry : file_extensions)
  {
    const auto& extension = entry.first;
    const auto extension_length = extension.length();

    if (length >= extension_length &&
        !filename.substr(length - extension_length).compare(extension))
    {
      return entry.second;
    }
  }

  return std::nullopt;
}

void NormPath(const char* in_path, char* out_path)
{
  auto result = std::filesystem::path(in_path).lexically_normal().string();

  // Match historic NormPath: drop a trailing separator except for root.
  if (result.size() > 1 &&
      (result.back() == '/' || result.back() == '\\'))
  {
    result.pop_back();
  }
  if (result.empty())
  {
    result = ".";
  }

  std::snprintf(out_path, PATH_LENGTH + 1, "%s", result.c_str());
}



/* reentrantes strtok */
char *Strtok_r( char *str, const char *delim, char **old )
{
  char *result;
  int  l, m;

  if( str == nullptr )
    str = *old;

  if( str == nullptr )
    return( nullptr );

  l = strlen( str );
  if( ( result = strtok( str, delim ) ) != nullptr ) {
    m = strlen( result );
    if( (m + 1) >= l)
      *old = nullptr;
    else
      *old = result + m + 1;

  } else
    *old = nullptr;

  return( result );
}




void GetMaxYX(WINDOW *win, int *height, int *width)
{
  if( win == dir_window )
  {
    *height = std::max(DIR_WINDOW_HEIGHT, 1);
    *width  = std::max(DIR_WINDOW_WIDTH, 1);
  }
  else if( win == small_file_window )
  {
    *height = std::max(FILE_WINDOW_1_HEIGHT, 1);
    *width  = std::max(FILE_WINDOW_1_WIDTH, 1);
  }
  else if( win == big_file_window )
  {
    *height = std::max(FILE_WINDOW_2_HEIGHT, 1);
    *width  = std::max(FILE_WINDOW_2_WIDTH, 1);
  }
  else if( win == f2_window )
  {
    *height = std::max(F2_WINDOW_HEIGHT - 1, 1); /* fake for separator line */
    *width  = std::max(F2_WINDOW_WIDTH, 1);
  }
  else if( win == history_window )
  {
    *height = std::max(HISTORY_WINDOW_HEIGHT, 1);
    *width  = std::max(HISTORY_WINDOW_WIDTH, 1);
  }
  else
  {
    Error("Unknown Window-ID*ABORT");
    std::exit(EXIT_FAILURE);
  }
}

std::optional<std::string> GetExtension(const std::string& filename)
{
  const auto pos = filename.rfind('.');

  if (pos == std::string::npos || pos == 0)
  {
    return std::nullopt;
  }

  return filename.substr(pos + 1);
}

std::string ShellEscape(const std::string& src)
{
  static const std::string esc_chars = "\\!\"#$&'()*;<>?[]^`{|}~";
  std::string result;

  for (const auto& c : src)
  {
    switch (c)
    {
      case '\t':
        result += "\\t";
        break;

      case '\n':
        result += "\\n";
        break;

      case '\r':
        result += "\\r";
        break;

      default:
        if (esc_chars.find(c) != std::string::npos)
        {
          result += '\\';
        }
        result += c;
    }
  }

  return result;
}

int BuildUserFileEntry(FileEntry *fe_ptr,
			int max_filename_len, int max_linkname_len,
			const char *tmpl, int linelen, char *line)
{
  char attributes[11];
  char modify_time[13];
  char change_time[13];
  char access_time[13];
  int  n;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  const char* sym_link_name = nullptr;
  const char* sptr;
  char* dptr;
  char tag;
  char buffer[4096]; /* enough??? */


  if( fe_ptr && S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    sym_link_name = fe_ptr->symlink_target.c_str();
  else
    sym_link_name = "";


  tag = (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ';
  (void) GetAttributes( fe_ptr->stat_struct.st_mode, attributes);

  (void) CTime( fe_ptr->stat_struct.st_mtime, modify_time );
  (void) CTime( fe_ptr->stat_struct.st_ctime, change_time );
  (void) CTime( fe_ptr->stat_struct.st_atime, access_time );

  if (const auto owner_name_ptr = GetPasswdName(fe_ptr->stat_struct.st_uid))
  {
    std::strncpy(owner, owner_name_ptr->c_str(), sizeof(owner));
  } else {
    std::snprintf(owner, sizeof(owner), "%d", fe_ptr->stat_struct.st_uid);
  }
  if (const auto group_name_ptr = GetPasswdName(fe_ptr->stat_struct.st_gid))
  {
    std::strncpy(group, group_name_ptr->c_str(), sizeof(group));
  } else {
    std::snprintf(group, sizeof(group), "%d", fe_ptr->stat_struct.st_gid);
  }

  const auto fitted_name = FitVisualWidth(fe_ptr->name, max_filename_len, true);
  const auto fitted_link = FitVisualWidth(sym_link_name, max_linkname_len, true);

  for(sptr=tmpl, dptr=buffer; *sptr; ) {

    if(*sptr == '%') {
      sptr++;
      if(std::string_view(sptr).starts_with(TAGSYMBOL_VIEWNAME)) {
        *dptr = tag; n=1;
      } else if(std::string_view(sptr).starts_with(FILENAME_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{}", fitted_name) - dptr);
      } else if(std::string_view(sptr).starts_with(ATTRIBUTE_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:>10}", attributes) - dptr);
      } else if(std::string_view(sptr).starts_with(LINKCOUNT_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:3}", (int)fe_ptr->stat_struct.st_nlink) - dptr);
      } else if(std::string_view(sptr).starts_with(FILESIZE_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:7}", (long long) fe_ptr->stat_struct.st_size) - dptr);
      } else if(std::string_view(sptr).starts_with(MODTIME_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:>12}", modify_time) - dptr);
      } else if(std::string_view(sptr).starts_with(SYMLINK_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{}", fitted_link) - dptr);
      } else if(std::string_view(sptr).starts_with(UID_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:<8}", owner) - dptr);
      } else if(std::string_view(sptr).starts_with(GID_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:<8}", group) - dptr);
      } else if(std::string_view(sptr).starts_with(INODE_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:7}", (long long)fe_ptr->stat_struct.st_ino) - dptr);
      } else if(std::string_view(sptr).starts_with(ACCTIME_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:>12}", access_time) - dptr);
      } else if(std::string_view(sptr).starts_with(CHGTIME_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:>12}", change_time) - dptr);
      } else {
	n = -1;
      }
      if(n == -1) {
        *dptr++ = '%';
	} else {
        dptr += n;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
      }
    } else {
      *dptr++ = *sptr++;
    }
  }
  *dptr = '\0';
  strncpy(line, buffer, linelen);
  line[linelen - 1] = '\0';
  return(0);
}



int GetUserFileEntryLength( int max_filename_len, int max_linkname_len, const char *tmpl)
{
  int  len, n;
  const char *sptr;


  for(len=0, sptr=tmpl; *sptr; ) {

    if(*sptr == '%') {
      sptr++;
      if(std::string_view(sptr).starts_with(TAGSYMBOL_VIEWNAME)) {
        n=1;
      } else if(std::string_view(sptr).starts_with(FILENAME_VIEWNAME)) {
        n = max_filename_len;
      } else if(std::string_view(sptr).starts_with(ATTRIBUTE_VIEWNAME)) {
        n = 10;
      } else if(std::string_view(sptr).starts_with(LINKCOUNT_VIEWNAME)) {
        n = 3;
      } else if(std::string_view(sptr).starts_with(FILESIZE_VIEWNAME)) {
        n = 7;
      } else if(std::string_view(sptr).starts_with(MODTIME_VIEWNAME)) {
        n = 12;
      } else if(std::string_view(sptr).starts_with(SYMLINK_VIEWNAME)) {
        n = max_linkname_len;
      } else if(std::string_view(sptr).starts_with(UID_VIEWNAME)) {
        n = 8;
      } else if(std::string_view(sptr).starts_with(GID_VIEWNAME)) {
        n = 8;
      } else if(std::string_view(sptr).starts_with(INODE_VIEWNAME)) {
        n = 7;
      } else if(std::string_view(sptr).starts_with(ACCTIME_VIEWNAME)) {
        n = 12;
      } else if(std::string_view(sptr).starts_with(CHGTIME_VIEWNAME)) {
        n = 12;
      } else {
	n = -1;
      }
      if(n == -1) {
        len++;
	sptr++;
	} else {
        len += n;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
      }
    } else {
      sptr++;
      len++;
    }
  }
  return(len);
}


long long AtoLL(const char *cptr)
{
  long long ll;

  sscanf(cptr, "%lld", &ll);

  return(ll);
}

std::optional<std::filesystem::path> Getcwd()
{
  std::error_code ec;
  auto path = std::filesystem::current_path(ec);

  if (ec)
  {
    return std::nullopt;
  }

  return path;
}

std::filesystem::path GetcwdOrDot()
{
  if (const auto cwd = Getcwd())
  {
    return *cwd;
  }
  Warning("Getcwd() failed*\".\" assumed");

  return std::filesystem::path(".");
}

static inline bool Stat(const std::string& path, struct stat& st)
{
#if defined(S_IFLNK) && !defined(isc386)
  return !lstat(path.c_str(), &st);
#else
  return !stat(path.c_str(), &st);
#endif
}

void StatOrAbort(const std::string& path, struct stat& st)
{
  if (!Stat(path, st))
  {
    Error("stat() failed*ABORT");
    std::exit(EXIT_FAILURE);
  }
}

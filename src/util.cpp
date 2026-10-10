#include "ytree.h"

#include <chrono>
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
  { ".tap.xz", CompressMethod::TAPE_DIR_XZ_COMPRESS },
  { ".TAP.XZ", CompressMethod::TAPE_DIR_XZ_COMPRESS },
  { ".tap.zst", CompressMethod::TAPE_DIR_ZSTD_COMPRESS },
  { ".TAP.ZST", CompressMethod::TAPE_DIR_ZSTD_COMPRESS },
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
  { ".xz", CompressMethod::XZ_COMPRESS },
  { ".XZ", CompressMethod::XZ_COMPRESS },
  { ".txz", CompressMethod::XZ_COMPRESS },
  { ".TXZ", CompressMethod::XZ_COMPRESS },
  { ".lzma", CompressMethod::XZ_COMPRESS },
  { ".LZMA", CompressMethod::XZ_COMPRESS },
  { ".zst", CompressMethod::ZSTD_COMPRESS },
  { ".ZST", CompressMethod::ZSTD_COMPRESS },
  { ".zstd", CompressMethod::ZSTD_COMPRESS },
  { ".ZSTD", CompressMethod::ZSTD_COMPRESS },
  { ".tzst", CompressMethod::ZSTD_COMPRESS },
  { ".TZST", CompressMethod::ZSTD_COMPRESS },
  { ".zoo", CompressMethod::ZOO_COMPRESS },
  { ".ZOO", CompressMethod::ZOO_COMPRESS },
  { ".lzh", CompressMethod::LHA_COMPRESS },
  { ".LZH", CompressMethod::LHA_COMPRESS },
  { ".arc", CompressMethod::ARC_COMPRESS },
  { ".ARC", CompressMethod::ARC_COMPRESS },
  { ".rar", CompressMethod::RAR_COMPRESS },
  { ".RAR", CompressMethod::RAR_COMPRESS },
  { ".7z", CompressMethod::SEVENZIP_COMPRESS },
  { ".7Z", CompressMethod::SEVENZIP_COMPRESS },
  { ".jar", CompressMethod::ZIP_COMPRESS },
  { ".zip", CompressMethod::ZIP_COMPRESS },
  { ".ZIP", CompressMethod::ZIP_COMPRESS },
  { ".JAR", CompressMethod::ZIP_COMPRESS },
  { ".rpm", CompressMethod::RPM_COMPRESS },
  { ".RPM", CompressMethod::RPM_COMPRESS },
  { ".spm", CompressMethod::RPM_COMPRESS },
  { ".SPM", CompressMethod::RPM_COMPRESS }
};

std::filesystem::path GetPath(const DirEntry* dir_entry)
{
  std::string result;

  /* Parents are owned by the tree, so the raw cursor stays valid while the
   * temporary shared_ptr returned by Parent() goes away. */
  for (const DirEntry* de_ptr = dir_entry; de_ptr; )
  {
    const auto parent = de_ptr->Parent();

    const auto is_root =
      de_ptr->name.size() == 1 &&
      de_ptr->name[0] == std::filesystem::path::preferred_separator;

    if (!is_root)
    {
      result.insert(0, de_ptr->name);
    }
    /* A nameless root (archive placeholder) doesn't contribute a separator */
    if (parent && !parent->name.empty())
    {
      result.insert(result.begin(), std::filesystem::path::preferred_separator);
    }

    de_ptr = parent.get();
  }

  return result;
}

std::filesystem::path GetFileNamePath(const FileEntry* file_entry)
{
  return GetPath(file_entry->Dir().get()) / file_entry->name;
}

std::filesystem::path GetRealFileNamePath(const FileEntry* file_entry)
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

    return GetPath(file_entry->Dir().get()) / file_entry->symlink_target;
  }

  return GetPath(file_entry->Dir().get()) / file_entry->name;
}

int GetDirEntry(
  const std::shared_ptr<DirEntry>& tree,
  DirEntry* current_dir_entry,
  const std::string& dir_path,
  DirEntry** dir_entry,
  std::string& to_path
)
{
  std::string dest_path;
  std::filesystem::path current_path;
  char *token, *old;
  DirEntry *de_ptr, *sde_ptr;
  int n;
  std::error_code ec;

  *dir_entry = nullptr;
  to_path = dir_path;

  if (const auto cwd = Getcwd())
  {
    current_path = *cwd;
  } else {
    Error("Getcwd failed");

    return -1;
  }

  const auto is_absolute =
    !dir_path.empty() &&
    dir_path.front() == std::filesystem::path::preferred_separator;

  if (!is_absolute)
  {
    std::filesystem::current_path(GetPath(current_dir_entry), ec);
    if (ec)
    {
      Error("chdir() failed");

      return -1;
    }
  }

  std::filesystem::current_path(dir_path, ec);
  if (ec)
  {
#ifdef DEBUG
    FormatMessage("Invalid Path!*\"{}\"", dir_path);
#endif
    return -3;
  }

  if (!is_absolute)
  {
    if (const auto cwd = Getcwd())
    {
      dest_path = *cwd;
    }
    to_path = dest_path;
  } else {
    dest_path = dir_path;
  }

  std::filesystem::current_path(current_path, ec);
  if (ec)
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
      (static_cast<int>(dest_path.size()) >= n &&
       tree->name.compare(0, n, dest_path, 0, n) == 0 &&
        ( static_cast<int>(dest_path.size()) == n ||
          dest_path[n] == std::filesystem::path::preferred_separator ) ) )
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree.get();
    token = Strtok_r(dest_path.data() + n, preferred_separator_str, &old);
    while( token )
    {
      sde_ptr = nullptr;
      for( const auto& child : de_ptr->children )
      {
        if( child->name == token )
  {
    /* Subtree gefunden */
    /*------------------*/

    sde_ptr = child.get();
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
      token = Strtok_r(nullptr, preferred_separator_str, &old);
    }
    *dir_entry = de_ptr;
  }
  return( 0 );
}




int GetFileEntry(
  DirEntry* de_ptr,
  const std::string& file_name,
  FileEntry** file_entry
)
{
  *file_entry = nullptr;

  for( const auto& fe_ptr : de_ptr->files )
  {
    if( fe_ptr->name == file_name )
    {
      /* Eintrag gefunden */
      /*------------------*/

      *file_entry = fe_ptr.get();
      break;
    }
  }
  return( 0 );
}





std::string GetAttributes(unsigned short modus)
{
  std::string buffer;
  buffer.reserve(10);

       if( S_ISREG(modus) )  buffer += '-';
  else if( S_ISDIR(modus) )  buffer += 'd';
  else if( S_ISCHR(modus) )  buffer += 'c';
  else if( S_ISBLK(modus) )  buffer += 'b';
  else if( S_ISFIFO(modus) ) buffer += 'p';
  else if( S_ISLNK(modus) )  buffer += 'l';
  else if( S_ISSOCK(modus) ) buffer += 's';  /* ??? */
  else                       buffer += '?';  /* unknown */

  if( modus & S_IRUSR ) buffer += 'r';
  else buffer += '-';

  if( modus & S_IWUSR ) buffer += 'w';
  else buffer += '-';

  if( modus & S_IXUSR ) buffer += 'x';
  else buffer += '-';

  if( modus & S_ISUID ) buffer.back() = 's';


  if( modus & S_IRGRP ) buffer += 'r';
  else buffer += '-';

  if( modus & S_IWGRP ) buffer += 'w';
  else buffer += '-';

  if( modus & S_IXGRP ) buffer += 'x';
  else buffer += '-';

  if( modus & S_ISGID ) buffer.back() = 's';


  if( modus & S_IROTH ) buffer += 'r';
  else buffer += '-';

  if( modus & S_IWOTH ) buffer += 'w';
  else buffer += '-';

  if( modus & S_IXOTH ) buffer += 'x';
  else buffer += '-';

  return buffer;
}

std::string CTime(std::time_t f_time)
{
  using namespace std::chrono;

  const auto tp = system_clock::from_time_t(f_time);
  const auto local = zoned_time{current_zone(), floor<seconds>(tp)};

  if (system_clock::now() - tp > seconds{31536000L})
  {
    /* Differenz groesser als 1 Jahr */
    /*-------------------------------*/

    return std::format("{:%b %e  %Y}", local);
  }

  return std::format("{:%b %e %H:%M}", local);
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

void PrintOptions(WINDOW *win, int y, int x, const std::string& str)
{
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

  for (const auto& c : str)
  {
    auto ch = static_cast<int>(c);

    switch (c)
    {
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
        default:  ch = PRINT(c);
     }

#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#else
    wattrset(win, color);
#endif
     mvwaddch(win, y, x++, ch);
     wattrset(win, 0);
   }
}


void PrintMenuOptions(
  WINDOW* win,
  int y,
  int x,
  const std::string& str,
  int ncolor,
  int hcolor
)
{
  int color;
  int hi_color;
  int lo_color;
  std::string sbuf;

  if (x < 0 || y < 0)
  {
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

  for (const auto& c : str)
  {
    auto ch = static_cast<int>(c);

    switch (ch)
    {
      case '(':
        color = hi_color;
        WAttrAddStr(
          win,
#ifdef COLOR_SUPPORT
          COLOR_PAIR(color) | A_BOLD,
#else
          color,
#endif
          sbuf
        );
        sbuf.clear();
        continue;

      case ')':
        color = lo_color;
        WAttrAddStr(
          win,
#ifdef COLOR_SUPPORT
          COLOR_PAIR(color) | A_BOLD,
#else
          color,
#endif
          sbuf
        );
        sbuf.clear();
        continue;

#ifdef COLOR_SUPPORT
      case ']':
        color = lo_color;
        WAttrAddStr(win, COLOR_PAIR(color) | A_BOLD, sbuf);
        sbuf.clear();
        continue;

      case '[':
        color = hi_color;
        WAttrAddStr(win, COLOR_PAIR(color) | A_BOLD, sbuf);
        sbuf.clear();
        continue;
#else
      case ']':
      case '[':
        /* ignore */
        continue;
#endif

      default:
        sbuf += static_cast<char>(PRINT(c));
    }
  }

  WAttrAddStr(
    win,
#ifdef COLOR_SUPPORT
    COLOR_PAIR(color) | A_BOLD,
#else
    color,
#endif
    sbuf
  );
}

std::string FormFilename(const std::string& src, std::size_t max_len)
{
  const auto l = src.size();

  if (l <= max_len)
  {
    return src;
  }

  std::size_t begin = 0;
  if (max_len > 4)
  {
    for (std::size_t i = 0; i < max_len - 4; ++i)
    {
      const char c = src[l - i];
      if (c == std::filesystem::path::preferred_separator || c == '\\')
      {
        begin = l - i;
      }
    }
  }

  return std::format("/...{}", src.substr(begin));
}

std::string CutFilename(const std::string& src, std::size_t max_len)
{
  const auto l = static_cast<std::size_t>(StrVisualLength(src));

  if (l <= max_len)
  {
    return src;
  }

  const auto tmp = StrLeft(src.c_str(), max_len - 3);
  return std::format("{}...", tmp);
}

std::string CutPathname(const std::string& src, std::size_t max_len)
{
  const auto l = src.length();

  if (l <= max_len)
  {
    return src;
  }

  return "..." + src.substr(l - max_len + 3);
}

/**
 * Split archive member path into parent directory (with trailing '/') and
 * basename.
 */
ArchivePathSplit Fnsplit(std::string path)
{
  ArchivePathSplit result;

  while (!path.empty() && (path.front() == ' ' || path.front() == '\t'))
  {
    path.erase(0, 1);
  }

  for (char& c : path)
  {
    if (c == '\\')
    {
      c = std::filesystem::path::preferred_separator;
    }
  }

  if (path.empty())
  {
    return result;
  }

  const std::filesystem::path parsed(path);
  result.name = parsed.filename().string();

  const auto parent = parsed.parent_path();
  if (!parent.empty() || parsed.has_root_path())
  {
    result.dir = parent.string();
    const char sep = std::filesystem::path::preferred_separator;
    if (result.dir.empty() || result.dir.back() != sep)
    {
      result.dir += sep;
    }
  }

  if (result.name.size() > PATH_LENGTH)
  {
    const auto full_name = result.name;

    result.name.resize(PATH_LENGTH);
    FormatWarning(
      "filename too long:*{}*truncating to*{}",
      full_name,
      result.name
    );
  }

  return result;
}

std::string BuildFilename(
  const std::string& in_filename,
  const std::string& pattern
)
{
  std::string out_filename;

  for (const char ch : pattern)
  {
    if (ch == '*')
    {
      out_filename += in_filename;
    }
    else
    {
      out_filename += ch;
    }
  }

  return out_filename;
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

std::string NormPath(const std::string& in_path)
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

  return result;
}



/* reentrantes strtok */
char *Strtok_r(char *str, const char *delim, char **old)
{
  char *result;
  int  l, m;

  if( str == nullptr )
    str = *old;

  if( str == nullptr )
    return( nullptr );

  l = std::strlen(str);
  if( ( result = std::strtok(str, delim) ) != nullptr ) {
    m = std::strlen(result);
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

std::string ShellQuote(const std::string& src)
{
  // Double-quoted shell word; escape only characters special inside "...".
  std::string result;

  result.reserve(src.size() + 2);
  result += '"';

  for (const auto c : src)
  {
    switch (c)
    {
      case '"':
      case '\\':
      case '$':
      case '`':
        result += '\\';
        [[fallthrough]];
      default:
        result += c;
        break;
    }
  }

  result += '"';

  return result;
}

int BuildUserFileEntry(
  FileEntry* fe_ptr,
  int max_filename_len,
  int max_linkname_len,
  const std::string& tmpl,
  int linelen,
  char* line
)
{
  int  n;
  const char* sym_link_name = nullptr;
  const char* sptr;
  char* dptr;
  char tag;
  char buffer[4096]; /* enough??? */


  if( fe_ptr && S_ISLNK(fe_ptr->stat_struct.st_mode) )
    sym_link_name = fe_ptr->symlink_target.c_str();
  else
    sym_link_name = "";


  tag = (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ';
  const auto attributes = GetAttributes(fe_ptr->stat_struct.st_mode);

  const auto modify_time = CTime(fe_ptr->stat_struct.st_mtime);
  const auto change_time = CTime(fe_ptr->stat_struct.st_ctime);
  const auto access_time = CTime(fe_ptr->stat_struct.st_atime);

  const auto owner = GetPasswdName(fe_ptr->stat_struct.st_uid)
    .value_or(std::to_string(fe_ptr->stat_struct.st_uid));
  const auto group = GetPasswdName(fe_ptr->stat_struct.st_gid)
    .value_or(std::to_string(fe_ptr->stat_struct.st_gid));

  const auto fitted_name = FitVisualWidth(fe_ptr->name, max_filename_len, true);
  const auto fitted_link = FitVisualWidth(sym_link_name, max_linkname_len, true);

  for(sptr = tmpl.c_str(), dptr = buffer; *sptr;)
  {
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
        n = static_cast<int>(std::format_to(dptr, "{:7}", (std::int64_t) fe_ptr->stat_struct.st_size) - dptr);
      } else if(std::string_view(sptr).starts_with(MODTIME_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:>12}", modify_time) - dptr);
      } else if(std::string_view(sptr).starts_with(SYMLINK_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{}", fitted_link) - dptr);
      } else if(std::string_view(sptr).starts_with(UID_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:<8}", owner) - dptr);
      } else if(std::string_view(sptr).starts_with(GID_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:<8}", group) - dptr);
      } else if(std::string_view(sptr).starts_with(INODE_VIEWNAME)) {
        n = static_cast<int>(std::format_to(dptr, "{:7}", (std::int64_t)fe_ptr->stat_struct.st_ino) - dptr);
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
  std::strncpy(line, buffer, linelen);
  line[linelen - 1] = '\0';
  return(0);
}



int GetUserFileEntryLength(
  int max_filename_len,
  int max_linkname_len,
  const std::string& tmpl
)
{
  int len;
  int n;
  const char* sptr;


  for (len = 0, sptr = tmpl.c_str(); *sptr;)
  {
    if (*sptr == '%') {
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

  return len;
}

std::int64_t AtoLL(const char *cptr)
{
  return static_cast<std::int64_t>(std::strtoll(cptr, nullptr, 10));
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
#ifdef S_IFLNK
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

#include <chrono>
#include <cstring>
#include <unordered_map>

#include "core/api.hpp"
#include "core/path.hpp"
#include "core/stat-macros.hpp"
#include "util/string-tokenizer.hpp"

static constexpr const char* TAGSYMBOL_VIEWNAME = "tag";
static constexpr const char* FILENAME_VIEWNAME = "fnm";
static constexpr const char* ATTRIBUTE_VIEWNAME = "atr";
static constexpr const char* LINKCOUNT_VIEWNAME = "lct";
static constexpr const char* FILESIZE_VIEWNAME = "fsz";
static constexpr const char* MODTIME_VIEWNAME = "mot";
static constexpr const char* SYMLINK_VIEWNAME = "lnm";
static constexpr const char* UID_VIEWNAME = "uid";
static constexpr const char* GID_VIEWNAME = "gid";
static constexpr const char* INODE_VIEWNAME = "ino";
static constexpr const char* ACCTIME_VIEWNAME = "act";
static constexpr const char* CHGTIME_VIEWNAME = "sct";

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

std::shared_ptr<DirEntry> SharedDirEntry(DirEntry* dir_entry)
{
  if (!dir_entry)
  {
    return nullptr;
  }

  if (const auto parent = dir_entry->Parent())
  {
    for (const auto& child : parent->children)
    {
      if (child.get() == dir_entry)
      {
        return child;
      }
    }
  }

  if (statistic.tree.get() == dir_entry)
  {
    return statistic.tree;
  }

  if (disk_statistic.tree.get() == dir_entry)
  {
    return disk_statistic.tree;
  }

  return nullptr;
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
  DirEntry *de_ptr, *sde_ptr;
  int n;
  std::error_code ec;

  *dir_entry = nullptr;
  to_path = dir_path;

  if (const auto cwd = Getcwd())
  {
    current_path = *cwd;
  } else {
    YTreeError("Getcwd failed");

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
      YTreeError("chdir() failed");

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
    YTreeError("chdir() failed; Can't resume");

    return -1;
  }

  n = tree->name.size();
  const char preferred_separator_str[]{
    std::filesystem::path::preferred_separator, '\0'};
  const auto tree_is_root = IsRoot(tree->name);

  if( tree_is_root ||
      (static_cast<int>(dest_path.size()) >= n &&
       tree->name.compare(0, n, dest_path, 0, n) == 0 &&
        ( static_cast<int>(dest_path.size()) == n ||
          dest_path[n] == std::filesystem::path::preferred_separator ) ) )
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree.get();
    StringTokenizer tokenizer(std::string_view(dest_path).substr(n));

    while (const auto token = tokenizer.Next(preferred_separator_str))
    {
      sde_ptr = nullptr;
      for (const auto& child : de_ptr->children)
      {
        if (child->name == *token)
        {
          /* Subtree gefunden */
          /*------------------*/

          sde_ptr = child.get();
          de_ptr = sde_ptr;
          break;
        }
      }
      if (sde_ptr == nullptr)
      {
#ifdef DEBUG
        FormatError("Can't find directory; token={}", *token);
#endif
        return -3;
      }
    }
    *dir_entry = de_ptr;
  }
  return 0;
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

namespace
{
constexpr std::string_view kMissingMeta = "-";
}  // namespace

std::string FormatMetaTime(const std::optional<EntryMeta::time_type>& tp)
{
  if (!tp)
  {
    return std::string(kMissingMeta);
  }
  return CTime(std::chrono::system_clock::to_time_t(*tp));
}

std::string FormatMetaOwner(const EntryMeta& meta)
{
  if (!meta.uid)
  {
    return std::string(kMissingMeta);
  }
  return GetPasswdName(*meta.uid).value_or(std::to_string(*meta.uid));
}

std::string FormatMetaGroup(const EntryMeta& meta)
{
  if (!meta.gid)
  {
    return std::string(kMissingMeta);
  }
  return GetGroupName(*meta.gid).value_or(std::to_string(*meta.gid));
}

std::string FormatMetaInode(const EntryMeta& meta)
{
  if (!meta.inode)
  {
    return std::string(kMissingMeta);
  }
  return std::to_string(*meta.inode);
}

std::string FormatMetaNlink(const EntryMeta& meta)
{
  if (!meta.nlink)
  {
    return std::string(kMissingMeta);
  }
  return std::to_string(*meta.nlink);
}

std::string CTime(std::time_t f_time)
{
#if defined(YTREE_HAS_STD_CHRONO_TZ)
  using namespace std::chrono;

  const auto tp = system_clock::from_time_t(f_time);
  const auto local = zoned_time{current_zone(), floor<seconds>(tp)};

  if(system_clock::now() - tp > seconds{31536000L})
  {
    /* Differenz groesser als 1 Jahr */
    /*-------------------------------*/

    return std::format("{:%b %e  %Y}", local);
  }

  return std::format("{:%b %e %H:%M}", local);
#else
  std::tm local_tm{};
  if(localtime_r(&f_time, &local_tm) == nullptr)
    return "?";

  const bool show_year = std::time(nullptr) - f_time > 31536000L;
  const char* format = show_year ? "%b %e  %Y" : "%b %e %H:%M";

  char buffer[64];
  if(std::strftime(buffer, sizeof(buffer), format, &local_tm) == 0)
    return "?";

  return buffer;
#endif
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

std::string BuildUserFileEntry(
  const std::shared_ptr<FileEntry>& fe_ptr,
  int max_filename_len,
  int max_linkname_len,
  const std::string& tmpl,
  int linelen
)
{
  std::string result;
  const char* sym_link_name = "";

  result.reserve(tmpl.size() + 256);

  if (fe_ptr && fe_ptr->meta.IsLink())
  {
    sym_link_name = fe_ptr->symlink_target.c_str();
  }

  const char tag = fe_ptr->tagged ? TAGGED_SYMBOL : ' ';
  const auto attributes = GetAttributes(fe_ptr->meta.Mode());

  const auto modify_time = FormatMetaTime(fe_ptr->meta.mtime);
  const auto change_time = FormatMetaTime(fe_ptr->meta.ctime);
  const auto access_time = FormatMetaTime(fe_ptr->meta.atime);

  const auto owner = FormatMetaOwner(fe_ptr->meta);
  const auto group = FormatMetaGroup(fe_ptr->meta);

  const auto fitted_name = FitVisualWidth(fe_ptr->name, max_filename_len, true);
  const auto fitted_link = FitVisualWidth(sym_link_name, max_linkname_len, true);

  for (std::size_t i = 0; i < tmpl.size();)
  {
    if (tmpl[i] != '%')
    {
      result += tmpl[i++];
      continue;
    }

    ++i;

    const auto rest = std::string_view(tmpl).substr(i);
    const auto append_and_skip = [&](const char* view_name, const auto& formatted)
    {
      if (!rest.starts_with(view_name))
      {
        return false;
      }
      result += formatted;
      i += std::strlen(view_name);

      return true;
    };

    if(append_and_skip(TAGSYMBOL_VIEWNAME, std::string(1, tag))
        || append_and_skip(FILENAME_VIEWNAME, std::format("{}", fitted_name))
        || append_and_skip(ATTRIBUTE_VIEWNAME, std::format("{:>10}", attributes))
        || append_and_skip(LINKCOUNT_VIEWNAME, std::format("{:>3}", FormatMetaNlink(fe_ptr->meta)))
        || append_and_skip(FILESIZE_VIEWNAME, std::format("{:7}", (std::int64_t) fe_ptr->meta.size))
        || append_and_skip(MODTIME_VIEWNAME, std::format("{:>12}", modify_time))
        || append_and_skip(SYMLINK_VIEWNAME, std::format("{}", fitted_link))
        || append_and_skip(UID_VIEWNAME, std::format("{:<8}", owner))
        || append_and_skip(GID_VIEWNAME, std::format("{:<8}", group))
        || append_and_skip(INODE_VIEWNAME, std::format("{:>7}", FormatMetaInode(fe_ptr->meta)))
        || append_and_skip(ACCTIME_VIEWNAME, std::format("{:>12}", access_time))
        || append_and_skip(CHGTIME_VIEWNAME, std::format("{:>12}", change_time))) {
    } else {
      result += '%';
    }
  }

  if(linelen > 0 && result.size() >= static_cast<std::size_t>(linelen))
  {
    result.resize(static_cast<std::size_t>(linelen - 1));
  }

  return result;
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

std::int64_t AtoLL(std::string_view s)
{
  return static_cast<std::int64_t>(
    std::strtoll(std::string(s).c_str(), nullptr, 10)
  );
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

EntryMeta MetaOrAbort(const std::filesystem::path& path)
{
  if (auto meta = EntryMeta::FromFilesystem(path))
  {
    return *meta;
  }

  YTreeError("stat() failed*ABORT");
  std::exit(EXIT_FAILURE);
  return {};
}

std::optional<std::string> ReadLine(FILE* f)
{
  std::string line;
  char buf[256];

  while (std::fgets(buf, sizeof(buf), f) != nullptr)
  {
    line.append(buf);
    if (!line.empty() && line.back() == '\n')
    {
      line.pop_back();
      if (!line.empty() && line.back() == '\r')
      {
        line.pop_back();
      }
      return line;
    }
  }

  if (line.empty())
  {
    return std::nullopt;
  }

  if (line.back() == '\r')
  {
    line.pop_back();
  }
  return line;
}

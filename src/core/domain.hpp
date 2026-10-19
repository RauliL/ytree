#pragma once

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include "config.hpp"
#include "core/entry_meta.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef S_IFLNK
#define readlink(a, b, c)  (-1)
#endif /* S_IFLNK */

#ifndef S_ISREG
#define S_ISREG(mode)   (((mode) & S_IFMT) == S_IFREG)
#endif /* S_ISREG */

#ifndef S_ISDIR
#define S_ISDIR(mode)   (((mode) & S_IFMT) == S_IFDIR)
#endif /* S_ISDIR */

#ifndef S_ISCHR
#define S_ISCHR(mode)   (((mode) & S_IFMT) == S_IFCHR)
#endif /* S_ISCHR */

#ifndef S_ISBLK
#define S_ISBLK(mode)   (((mode) & S_IFMT) == S_IFBLK)
#endif /* S_ISBLK */

#ifndef S_ISFIFO
#define S_ISFIFO(mode)   (((mode) & S_IFMT) == S_IFIFO)
#endif /* S_ISFIFO */

#ifndef S_ISLNK
#ifdef  S_IFLNK
#define S_ISLNK(mode)   (((mode) & S_IFMT) == S_IFLNK)
#else
#define S_ISLNK(mode)   FALSE
#endif /* S_IFLNK */
#endif /* S_ISLNK */

#ifndef S_ISSOCK
#ifdef  S_IFSOCK
#define S_ISSOCK(mode)   (((mode) & S_IFMT) == S_IFSOCK)
#else
#define S_ISSOCK(mode)   FALSE
#endif /* S_IFSOCK */
#endif /* S_ISSOCK */

inline constexpr const char* DEFAULT_TREE = ".";

#define Error(msg) ErrorEx(msg, __FILE__, __LINE__)
#define FormatError(...) ErrorEx(std::format(__VA_ARGS__), __FILE__, __LINE__)
#define FormatWarning(...) Warning(std::format(__VA_ARGS__))
#define FormatMessage(...) Message(std::format(__VA_ARGS__))

inline constexpr char TAGGED_SYMBOL = '*';

enum class Mode : int
{
  DISK_MODE = 0,
  LL_FILE_MODE = 1,
  TAR_FILE_MODE = 2,
  ZOO_FILE_MODE = 3,
  ZIP_FILE_MODE = 4,
  LHA_FILE_MODE = 5,
  ARC_FILE_MODE = 6,
  RPM_FILE_MODE = 7,
  RAR_FILE_MODE = 8,
  TAPE_MODE = 9,
  USER_MODE = 10,
  SEVENZIP_FILE_MODE = 11,
};

inline constexpr int MAX_MODES = 12;

enum class CompressMethod : int
{
  FREEZE_COMPRESS = 1,
  MULTIPLE_FREEZE_COMPRESS = 2,
  COMPRESS_COMPRESS = 3,
  MULTIPLE_COMPRESS_COMPRESS = 4,
  GZIP_COMPRESS = 5,
  BZIP_COMPRESS = 6,
  MULTIPLE_GZIP_COMPRESS = 7,
  ZOO_COMPRESS = 8,
  LHA_COMPRESS = 9,
  ARC_COMPRESS = 10,
  ZIP_COMPRESS = 11,
  RPM_COMPRESS = 12,
  TAPE_DIR_NO_COMPRESS = 13,
  TAPE_DIR_FREEZE_COMPRESS = 14,
  TAPE_DIR_COMPRESS_COMPRESS = 15,
  TAPE_DIR_GZIP_COMPRESS = 16,
  TAPE_DIR_BZIP_COMPRESS = 17,
  RAR_COMPRESS = 18,
  SEVENZIP_COMPRESS = 19,
  XZ_COMPRESS = 20,
  ZSTD_COMPRESS = 21,
  TAPE_DIR_XZ_COMPRESS = 22,
  TAPE_DIR_ZSTD_COMPRESS = 23,
};

enum class SortKey
{
  Name,
  ModTime,
  ChgTime,
  AccTime,
  Size,
  Owner,
  Group,
  Extension,
};

enum class SortOrder
{
  Ascending,
  Descending,
};

struct SortSpec
{
  SortKey key = SortKey::Name;
  SortOrder order = SortOrder::Ascending;
};

inline constexpr const char* DEFAULT_FILE_SPEC = "*";

inline constexpr const char* TAGSYMBOL_VIEWNAME = "tag";
inline constexpr const char* FILENAME_VIEWNAME = "fnm";
inline constexpr const char* ATTRIBUTE_VIEWNAME = "atr";
inline constexpr const char* LINKCOUNT_VIEWNAME = "lct";
inline constexpr const char* FILESIZE_VIEWNAME = "fsz";
inline constexpr const char* MODTIME_VIEWNAME = "mot";
inline constexpr const char* SYMLINK_VIEWNAME = "lnm";
inline constexpr const char* UID_VIEWNAME = "uid";
inline constexpr const char* GID_VIEWNAME = "gid";
inline constexpr const char* INODE_VIEWNAME = "ino";
inline constexpr const char* ACCTIME_VIEWNAME = "act";
inline constexpr const char* CHGTIME_VIEWNAME = "sct";

inline constexpr std::time_t CLOCK_INTERVAL = 1;

inline constexpr const char* ERR_TO_NULL = " 2> /dev/null";
inline constexpr const char* ERR_TO_STDOUT = " 2>&1 ";

inline constexpr int CR = 13;
inline constexpr int LF = 10;
inline constexpr int ESC = 27;
inline constexpr char LOGIN_ESC = '.';

inline constexpr std::size_t PATH_LENGTH = 1024;
inline constexpr std::size_t FILE_SPEC_LENGTH = 12 + 1;
inline constexpr std::size_t DISK_NAME_LENGTH = 12 + 1;

enum class ViewMode : int
{
  MODE_1 = 0,
  MODE_2 = 1,
  MODE_3 = 2,
  MODE_4 = 3,
  MODE_5 = 4,
};

inline constexpr int QUICK_BAUD_RATE = 9600;

template<class Fn>
class ScopeGuard
{
public:
  explicit ScopeGuard(Fn fn)
    : fn_(std::move(fn)) {}

  ScopeGuard(ScopeGuard&& other) noexcept
    : fn_(std::move(other.fn_))
    , engaged_(other.engaged_)
  {
    other.engaged_ = false;
  }

  ~ScopeGuard()
  {
    if (engaged_)
      fn_();
  }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;

  void release()
  {
    engaged_ = false;
  }

private:
  Fn fn_;
  bool engaged_{true};
};

template<class Fn>
ScopeGuard(Fn) -> ScopeGuard<Fn>;

struct Entry
{
  std::string name;
  EntryMeta meta{};

  virtual ~Entry() = default;

  virtual std::filesystem::path GetPath() const = 0;

  bool ChangeModus();

  bool SetModus(const std::string& new_modus_str);

  bool ChangeOwner();

  bool SetOwner(int new_user_id);

  bool ChangeGroup();

  bool SetGroup(int new_group_id);

  virtual bool Delete() = 0;
};

struct DirEntry;

/*
 * Ownership model:
 *  - Statistic::tree owns the root DirEntry.
 *  - DirEntry::children / DirEntry::files own their entries (shared_ptr).
 *  - DirEntry::parent and FileEntry::dir_entry are weak back-references
 *    (no ownership, no reference cycles).
 *  - Raw DirEntry* / FileEntry* in function signatures are short-lived,
 *    non-owning observers obtained from a shared_ptr.
 *  - SharedDirEntry() recovers the owning shared_ptr from a raw DirEntry*
 *    (via parent->children, or Statistic::tree / disk_statistic.tree).
 */
struct FileEntry : Entry
{
  /** Back reference to owning directory. */
  std::weak_ptr<DirEntry> dir_entry;
  bool tagged = false;
  bool matching = false;
  std::string symlink_target;

  /**
   * Owning directory, or null if it has expired.
   */
  inline std::shared_ptr<DirEntry> Dir() const
  {
    return dir_entry.lock();
  }

  std::filesystem::path GetPath() const override;

  std::filesystem::path GetRealPath() const;

  std::optional<std::string> GetExtension() const
  {
    const auto extension = std::filesystem::path(name).extension();

    if (extension.empty())
    {
      return std::nullopt;
    }

    return extension.string();
  }

  bool Delete() override;

  bool Remove();

  bool Pipe() const;

  bool PipeTo(FILE* pipe) const;
};

struct DirEntry : Entry
{
  std::vector<std::shared_ptr<FileEntry>> files;
  std::vector<std::shared_ptr<DirEntry>> children;
  std::weak_ptr<DirEntry> parent;
  std::int64_t total_bytes = 0;
  std::int64_t matching_bytes = 0;
  std::int64_t tagged_bytes = 0;
  unsigned int total_files = 0;
  unsigned int matching_files = 0;
  unsigned int tagged_files = 0;
  int cursor_pos = 0;
  int start_file = 0;
  bool access_denied = false;
  bool global_flag = false;
  bool tagged_flag = false;
  bool only_tagged = false;
  bool not_scanned = false;
  bool big_window = false;
  bool login_flag = false;

  /**
   * Parent directory, or null for the root (or if it has expired).
   */
  inline std::shared_ptr<DirEntry> Parent() const
  {
    return parent.lock();
  }

  std::filesystem::path GetPath() const override;

  void SetMatchingParam();

  bool Delete() override;

  void UnReadTree();
};

struct Statistic
{
  std::shared_ptr<DirEntry> tree;
  std::int64_t disk_space;
  std::int64_t disk_capacity;
  std::int64_t disk_total_files;
  std::int64_t disk_total_bytes;
  std::int64_t disk_matching_files;
  std::int64_t disk_matching_bytes;
  std::int64_t disk_tagged_files;
  std::int64_t disk_tagged_bytes;
  unsigned int  disk_total_directories;
  int           disp_begin_pos;
  int           cursor_pos;
  SortSpec      kind_of_sort;
  std::string   login_path;
  std::string   path;
  std::string   tape_name;
  std::string   file_spec;
  std::string   disk_name;
};

struct ArchivePathSplit
{
  std::string dir;
  std::string name;
};

struct AsToParameter
{
  std::string to_file;
  std::string to_dir;
};


extern Statistic statistic;
extern Statistic disk_statistic;
extern Mode      mode;
extern int       user_umask;
extern bool      print_time;
extern bool      resize_request;
extern bool      bypass_small_window;
extern std::optional<std::string> initial_directory;

/* Non-destructive strtok replacement: splits a string_view on delimiters. */
class StringTokenizer
{
public:
  explicit StringTokenizer(std::string_view input) : rest_(input) {}

  /**
   * Next token. An empty `delimiters` string means "remainder of the line"
   * (filenames may contain spaces), matching classic Strtok_r(..., "").
   */
  std::optional<std::string_view> Next(std::string_view delimiters)
  {
    while (!rest_.empty() &&
           !delimiters.empty() &&
           delimiters.find(rest_.front()) != std::string_view::npos)
    {
      rest_.remove_prefix(1);
    }

    if (rest_.empty())
    {
      return std::nullopt;
    }

    if (delimiters.empty())
    {
      const auto token = rest_;
      rest_ = {};
      return token;
    }

    const auto pos = rest_.find_first_of(delimiters);
    std::string_view token;

    if (pos == std::string_view::npos)
    {
      token = rest_;
      rest_ = {};
    }
    else
    {
      token = rest_.substr(0, pos);
      rest_.remove_prefix(pos + 1);
    }

    return token;
  }

private:
  std::string_view rest_;
};

inline int SvAtoi(std::string_view s)
{
  return static_cast<int>(std::strtol(std::string(s).c_str(), nullptr, 10));
}

std::optional<std::string> GetProfileValue(const std::string& key);

inline std::string GetProfileValueOrEmpty(const std::string& key)
{
  return GetProfileValue(key).value_or(std::string());
}

inline bool IsReadable(const std::filesystem::path& path)
{
  return !access(path.c_str(), R_OK);
}

inline bool IsWriteable(const std::filesystem::path& path)
{
  return !access(path.c_str(), W_OK);
}

template<class... Args>
std::string Join(Args&&... args)
{
  std::string result;
  bool first = true;
  auto append = [&result, &first](const std::string& part)
  {
    if (first)
    {
      first = false;
    } else {
      result.append(1, ' ');
    }
    result.append(part);
  };

  (append(args), ...);

  return result;
}

// Forward decls used by Format* macros / domain helpers.
void ErrorEx(const std::string& msg, const std::string& module, int line);
void Warning(const std::string& msg);
void Message(const std::string& msg);

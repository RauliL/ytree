#pragma once

#include <format>

#include "config.hpp"
#include "core/dir-entry.hpp"

#if 0
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define YTreeError(msg) ErrorEx(msg, __FILE__, __LINE__)
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

inline constexpr const char* ERR_TO_STDOUT = " 2>&1 ";

inline constexpr int CR = 13;
inline constexpr int LF = 10;
inline constexpr int ESC = 27;
inline constexpr char LOGIN_ESC = '.';

inline constexpr std::size_t PATH_LENGTH = 1024;

enum class ViewMode : int
{
  MODE_1 = 0,
  MODE_2 = 1,
  MODE_3 = 2,
  MODE_4 = 3,
  MODE_5 = 4,
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

inline int SvAtoi(std::string_view s)
{
  return static_cast<int>(std::strtol(std::string(s).c_str(), nullptr, 10));
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

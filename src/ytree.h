#pragma once

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include "./config.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <clocale>
#include <cmath>
#include <cstdint>
#include <csetjmp>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#if defined(WITH_UTF8)
# include <cwchar>
#endif
#include <algorithm>
#include <filesystem>
#include <format>
#include <memory>
#include <new>
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

#if defined(WITH_UTF8) && __has_include(<ncursesw/ncurses.h>)
# include <ncursesw/ncurses.h>
#elif defined(WITH_UTF8) && __has_include(<ncursesw/curses.h>)
# include <ncursesw/curses.h>
#elif defined(CURSES_HAVE_NCURSES_NCURSES_H)
# include <ncurses/ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_H)
# include <ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_CURSES_H)
# include <ncurses/curses.h>
#else
# include <curses.h>
#endif

#ifndef KEY_BTAB
#define KEY_BTAB  0x1d
#endif

#ifndef KEY_END
#define KEY_END   KEY_EOL
#endif

#ifdef S_IFLNK
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
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


#define VI_KEY_UP    'k'
#define VI_KEY_DOWN  'j'
#define VI_KEY_RIGHT 'l'
#define VI_KEY_LEFT  'h'
#define VI_KEY_NPAGE ( 'D' & 0x1F )
#define VI_KEY_PPAGE ( 'U' & 0x1F )

static constexpr std::size_t OWNER_NAME_MAX = 64;
static constexpr std::size_t GROUP_NAME_MAX = 64;
static constexpr std::size_t DISPLAY_OWNER_NAME_MAX = 12;
static constexpr std::size_t DISPLAY_GROUP_NAME_MAX = 12;

/* Sonderzeichen fuer Liniengrafik */
/*---------------------------------*/

#ifndef ACS_ULCORNER
#define ACS_ULCORNER '+'
#endif
#ifndef ACS_URCORNER
#define ACS_URCORNER '+'
#endif
#ifndef ACS_LLCORNER
#define ACS_LLCORNER '+'
#endif
#ifndef ACS_LRCORNER
#define ACS_LRCORNER '+'
#endif
#ifndef ACS_VLINE
#define ACS_VLINE    '|'
#endif
#ifndef ACS_HLINE
#define ACS_HLINE    '-'
#endif
#ifndef ACS_RTEE
#define ACS_RTEE    '+'
#endif
#ifndef ACS_LTEE
#define ACS_LTEE    '+'
#endif
#ifndef ACS_BTEE
#define ACS_BTEE    '+'
#endif
#ifndef ACS_TTEE
#define ACS_TTEE    '+'
#endif
#ifndef ACS_BLOCK
#define ACS_BLOCK   '?'
#endif
#ifndef ACS_LARROW
#define ACS_LARROW  '<'
#endif



/* Color Definitionen */

#define DIR_COLOR        1
#define FILE_COLOR       2
#define STATS_COLOR      3
#define BORDERS_COLOR    4
#define MENU_COLOR       5
#define WINDIR_COLOR     6
#define WINFILE_COLOR    7
#define WINSTATS_COLOR   8
#define WINERR_COLOR     9
#define HIDIR_COLOR     10
#define HIFILE_COLOR    11
#define HISTATS_COLOR   12
#define HIMENUS_COLOR   13
#define WINHST_COLOR    14
#define HST_COLOR       15
#define HIHST_COLOR     16
#define WINMTCH_COLOR   14
#define MTCH_COLOR      15
#define HIMTCH_COLOR    16
#define GLOBAL_COLOR    17
#define HIGLOBAL_COLOR  18

/* Auswahl der benutzten UNIX-Kommandos */
/*--------------------------------------*/

#define DEFAULT_TREE       "."

#define Error(msg) ErrorEx(msg, __FILE__, __LINE__)
#define FormatError(...) ErrorEx(std::format(__VA_ARGS__), __FILE__, __LINE__)
#define FormatWarning(...) Warning(std::format(__VA_ARGS__))
#define FormatMessage(...) Message(std::format(__VA_ARGS__))

#define TAGGED_SYMBOL '*'

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
};

inline constexpr int MAX_MODES = 11;

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

#define DEFAULT_FILE_SPEC "*"

#define TAGSYMBOL_VIEWNAME  "tag"
#define FILENAME_VIEWNAME "fnm"
#define ATTRIBUTE_VIEWNAME  "atr"
#define LINKCOUNT_VIEWNAME  "lct"
#define FILESIZE_VIEWNAME "fsz"
#define MODTIME_VIEWNAME  "mot"
#define SYMLINK_VIEWNAME  "lnm"
#define UID_VIEWNAME    "uid"
#define GID_VIEWNAME    "gid"
#define INODE_VIEWNAME    "ino"
#define ACCTIME_VIEWNAME  "act"
#define CHGTIME_VIEWNAME  "sct"

static constexpr std::time_t CLOCK_INTERVAL = 1;

#define ERR_TO_NULL           " 2> /dev/null"
#define ERR_TO_STDOUT         " 2>&1 "

#define LF         10
#define ESC        27
#define LOGIN_ESC  '.'

#define CR                     13

#define DIR_WINDOW_X         1
#define DIR_WINDOW_Y         2
#define DIR_WINDOW_WIDTH     (COLS - 26)
#define DIR_WINDOW_HEIGHT    ((LINES * 8 / 14)-1)

#define F2_WINDOW_X          DIR_WINDOW_X
#define F2_WINDOW_Y          DIR_WINDOW_Y
#define F2_WINDOW_WIDTH      DIR_WINDOW_WIDTH
#define F2_WINDOW_HEIGHT     (DIR_WINDOW_HEIGHT + 1)

#define FILE_WINDOW_1_X      1
#define FILE_WINDOW_1_Y      DIR_WINDOW_HEIGHT + 3
#define FILE_WINDOW_1_WIDTH  (COLS - 26)
#define FILE_WINDOW_1_HEIGHT (LINES - DIR_WINDOW_HEIGHT - 7 )

#define FILE_WINDOW_2_X      1
#define FILE_WINDOW_2_Y      2
#define FILE_WINDOW_2_WIDTH  (COLS - 26)
#define FILE_WINDOW_2_HEIGHT (LINES - 6)

#define ERROR_WINDOW_WIDTH   40
#define ERROR_WINDOW_HEIGHT  10
#define ERROR_WINDOW_X       ((COLS - ERROR_WINDOW_WIDTH) >> 1)
#define ERROR_WINDOW_Y       ((LINES - ERROR_WINDOW_HEIGHT) >> 1)

#define HISTORY_WINDOW_X       1
#define HISTORY_WINDOW_Y       2
#define HISTORY_WINDOW_WIDTH   (COLS - 26)
#define HISTORY_WINDOW_HEIGHT  (LINES - 6)

#define MATCHES_WINDOW_X       1
#define MATCHES_WINDOW_Y       2
#define MATCHES_WINDOW_WIDTH   (COLS - 26)
#define MATCHES_WINDOW_HEIGHT  (LINES - 6)

#define TIME_WINDOW_X        ((COLS > 20) ? (COLS - 20) : 1)
#define TIME_WINDOW_Y        1
#define TIME_WINDOW_WIDTH    ((COLS > 15) ? 15 : COLS)
#define TIME_WINDOW_HEIGHT   1

static constexpr std::size_t PATH_LENGTH = 1024;
static constexpr std::size_t FILE_SPEC_LENGTH = 12 + 1;
static constexpr std::size_t DISK_NAME_LENGTH = 12 + 1;
static constexpr std::size_t LL_LINE_LENGTH = 512;
static constexpr std::size_t TAR_LINE_LENGTH = 512;
static constexpr std::size_t RPM_LINE_LENGTH = 512;
static constexpr std::size_t ZOO_LINE_LENGTH = 512;
static constexpr std::size_t ZIP_LINE_LENGTH = 512;
static constexpr std::size_t LHA_LINE_LENGTH = 512;
static constexpr std::size_t ARC_LINE_LENGTH = 512;
static constexpr std::size_t RAR_LINE_LENGTH = 512;
static constexpr std::size_t COMMAND_LINE_LENGTH = 4096;

enum class ViewMode : int
{
  MODE_1 = 0,
  MODE_2 = 1,
  MODE_3 = 2,
  MODE_4 = 3,
  MODE_5 = 4,
};

static constexpr int QUICK_BAUD_RATE = 9600;

#define ESCAPE               goto FNC_XIT

#define PRINT(ch) (std::iscntrl(ch) && (((unsigned char)(ch)) < ' ')) ? (ACS_BLOCK) : ((unsigned char)(ch))
/* #define PRINT(ch) (ch) */

#ifdef COLOR_SUPPORT
extern void StartColors();
extern void WbkgdSet(WINDOW *w, chtype c);
#else
#define StartColors() ;
#define WbkgdSet(a, b)  ;
#endif /* COLOR_SUPPORT */

struct DirEntry;

/*
 * Ownership model:
 *  - Statistic::tree owns the root DirEntry.
 *  - DirEntry::children / DirEntry::files own their entries (shared_ptr).
 *  - DirEntry::parent and FileEntry::dir_entry are weak back-references
 *    (no ownership, no reference cycles).
 *  - Raw DirEntry* / FileEntry* in function signatures are short-lived,
 *    non-owning observers obtained from a shared_ptr.
 */
struct FileEntry
{
  std::weak_ptr<DirEntry> dir_entry; /* back-reference to owning directory */
  struct stat stat_struct{};
  bool tagged = false;
  bool matching = false;
  std::string name;
  std::string symlink_target;

  /* Owning directory, or null if it has expired. */
  std::shared_ptr<DirEntry> Dir() const { return dir_entry.lock(); }

  std::optional<std::string> GetExtension() const
  {
    const auto extension = std::filesystem::path(name).extension();

    if (extension.empty())
    {
      return std::nullopt;
    }

    return extension.string();
  }
};

struct DirEntry : std::enable_shared_from_this<DirEntry>
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
  struct stat stat_struct{};
  bool access_denied = false;
  bool global_flag = false;
  bool tagged_flag = false;
  bool only_tagged = false;
  bool not_scanned = false;
  bool big_window = false;
  bool login_flag = false;
  std::string name;

  /* Parent directory, or null for the root (or if it has expired). */
  std::shared_ptr<DirEntry> Parent() const { return parent.lock(); }
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
  char          login_path[PATH_LENGTH + 1];
  char          path[PATH_LENGTH + 1];
  char          tape_name[PATH_LENGTH + 1];
  char          file_spec[FILE_SPEC_LENGTH + 1];
  char          disk_name[DISK_NAME_LENGTH + 1];
};

struct WalkContextBase
{
  FileEntry *new_fe_ptr = nullptr;
};

struct ChangeModusWalkContext : WalkContextBase
{
  char new_modus[11]{};
};

struct ChangeOwnerWalkContext : WalkContextBase
{
  unsigned new_owner_id = 0;
};

struct ChangeGroupWalkContext : WalkContextBase
{
  unsigned new_group_id = 0;
};

struct ExecuteWalkContext : WalkContextBase
{
  std::string command;
};

struct CopyWalkContext : WalkContextBase
{
  Statistic *statistic_ptr = nullptr;
  DirEntry  *dest_dir_entry = nullptr;
  char      *to_file = nullptr;
  char      *to_path = nullptr;
  bool      path_copy = false;
  bool      confirm = false;
};

struct RenameWalkContext : WalkContextBase
{
  char *new_name = nullptr;
  bool confirm = false;
};

struct MoveWalkContext : WalkContextBase
{
  DirEntry *dest_dir_entry = nullptr;
  char     *to_file = nullptr;
  char     *to_path = nullptr;
  bool     confirm = false;
};

struct PipeWalkContext : WalkContextBase
{
  FILE *pipe_file = nullptr;
};

union FunctionData
{
  struct
  {
    char      new_modus[11];
  } change_modus;

  struct
  {
    unsigned  new_owner_id;
  } change_owner;

  struct
  {
    unsigned  new_group_id;
  } change_group;

  struct
  {
    char      *command;
  } execute;

  struct
  {
    Statistic *statistic_ptr;
    DirEntry  *dest_dir_entry;
    char      *to_file;
    char      *to_path;
    bool      path_copy;
    bool      confirm;
  } copy;

  struct
  {
    char      *new_name;
    bool      confirm;
  } rename;

  struct
  {
    DirEntry  *dest_dir_entry;
    char      *to_file;
    char      *to_path;
    bool      confirm;
  } mv;

  struct
  {
    FILE      *pipe_file;
  } pipe_cmd;

  struct
  {
   FILE       *zipfile;
   int        method;
   } compress_cmd;
};

struct WalkingPackage
{
  FileEntry     *new_fe_ptr;
  FunctionData  function_data;
};

extern WINDOW *dir_window;
extern WINDOW *small_file_window;
extern WINDOW *big_file_window;
extern WINDOW *file_window;
extern WINDOW *error_window;
extern WINDOW *history_window;
extern WINDOW *matches_window;
extern WINDOW *f2_window;
extern WINDOW *time_window;

extern Statistic statistic;
extern Statistic disk_statistic;
extern Mode      mode;
extern int       user_umask;
extern bool  print_time;
extern bool      resize_request;
extern bool      bypass_small_window;
extern std::optional<std::string> initial_directory;
extern char    builtin_hexdump_cmd[];

extern void DisplayMenu();
extern void DisplayDiskStatistic();
extern void DisplayDirStatistic(DirEntry *dir_entry);
extern void DisplayDirParameter(DirEntry *dir_entry);
extern void DisplayDirTagged(DirEntry *dir_entry);
extern void DisplayDiskTagged();
extern void DisplayDiskName();
extern void DisplayFileParameter(FileEntry *file_entry);
extern void DisplayGlobalFileParameter(FileEntry *file_entry);
void RefreshWindow(WINDOW* win);
int ReadTree(const std::shared_ptr<DirEntry>& dir_entry, const std::string& path, int depth);
extern void UnReadTree(DirEntry *dir_entry);
extern int  ReadTreeFromTAR(const std::shared_ptr<DirEntry>& dir_entry, FILE *f);
extern int  ReadTreeFromRPM(const std::shared_ptr<DirEntry>& dir_entry, FILE *f);
extern int  ReadTreeFromZOO(const std::shared_ptr<DirEntry>& dir_entry, FILE *f);
extern int  ReadTreeFromZIP(const std::shared_ptr<DirEntry>& dir_entry, FILE *f);
extern int  ReadTreeFromLHA(const std::shared_ptr<DirEntry>& dir_entry, FILE *f);
extern int  ReadTreeFromARC(const std::shared_ptr<DirEntry>& dir_entry, FILE *f);
extern int  ReadTreeFromRAR(const std::shared_ptr<DirEntry>& dir_entry, FILE *f);
extern int  GetDiskParameter(const std::string& path,
           char *volume_name,
           std::int64_t *avail_bytes,
           std::int64_t *capacity
);
extern int  HandleDirWindow(DirEntry *start_dir_entry);
extern void DisplayFileWindow(DirEntry *dir_entry);
void Init(
  const std::optional<std::filesystem::path>& configuration_file,
  const std::optional<std::filesystem::path>& history_file
);
std::filesystem::path GetPath(const DirEntry* dir_entry);
bool Match(const std::string& file_name);
int SetMatchSpec(const std::string& new_spec);
extern int  SetFileSpec(char *file_spec);
extern void SetMatchingParam(DirEntry *dir_entry);
void ErrorEx(const std::string& msg, const std::string& module, int line);
void Warning(const std::string& msg);
void UnmapNoticeWindow();
extern void SetFileMode(ViewMode new_file_mode);
extern int  HandleFileWindow(DirEntry *dir_entry);
extern char *GetAttributes(unsigned short modus, char *buffer);
extern void SwitchToSmallFileWindow();
extern void SwitchToBigFileWindow();
std::optional<std::string> GetGroupName(gid_t gid);
std::optional<int> GetGroupId(const std::string& name);
std::optional<std::string> GetPasswdName(uid_t uid);
std::optional<int> GetPasswdUid(const std::string& name);
std::filesystem::path GetFileNamePath(const FileEntry* file_entry);
std::filesystem::path GetRealFileNamePath(const FileEntry* file_entry);
int SystemCall(const std::string& command_line);
int QuerySystemCall(const std::string& command_line);
int SilentSystemCall(const std::string& command_line);
int SilentSystemCallEx(const std::string& command_line, bool enable_clock);
int View(DirEntry* dir_entry, const std::filesystem::path& file_path);
int ViewHex(const std::filesystem::path& file_path);
int InternalView(const std::filesystem::path& file_path);
int Edit(const DirEntry* dir_entry, const std::string& file_path);
extern void DisplayAvailBytes();
extern void DisplayFileSpec();
extern void QuitTo(DirEntry * dir_entry);
void Quit();
extern int  ReadFileSpec();
int InputString(
  std::string& s,
  const int y,
  const int x,
  const std::size_t initial_pos,
  const std::size_t max_length
);
extern void RotateFileMode();
int Execute(const DirEntry* dir_entry, const FileEntry* file_entry);
extern int  Pipe(DirEntry *dir_entry, FileEntry *file_entry);
extern int  PipeTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  GetPipeCommand(char *pipe_command);
extern void GetKindOfSort();
extern void SetKindOfSort(SortKey key, SortOrder order = SortOrder::Ascending);
extern int  ChangeFileModus(FileEntry *fe_ptr);
extern int  ChangeDirModus(DirEntry *de_ptr);
extern int  GetNewFileModus(int y, int x, char *modus, const char *term);
extern int  GetModus(const char *modus);
extern int  SetFileModus(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  CopyTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  CopyFile(Statistic *statistic_ptr, FileEntry *fe_ptr, bool confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, bool path_copy);
extern int  MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  MoveFile(FileEntry *fe_ptr, bool confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, FileEntry **new_fe_ptr);
extern int  InputChoise(const char *msg, const char *term);
void Message(const std::string& msg);
extern int  GetDirEntry(const std::shared_ptr<DirEntry>& tree, DirEntry *current_dir_entry, char *dir_path, DirEntry **dir_entry, char *to_path);
extern int  GetFileEntry(DirEntry *de_ptr, char *file_name, FileEntry **file_entry);
extern int  GetCopyParameter(const char *from_file, bool path_copy, char *to_file, char *to_dir);
extern int  GetMoveParameter(const char *from_file, char *to_file, char *to_dir);
extern int  ChangeFileOwner(FileEntry *fe_ptr);
extern int  GetNewOwner(int st_uid);
extern int  SetFileOwner(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  ChangeDirOwner(DirEntry *de_ptr);
extern int  ChangeFileGroup(FileEntry *fe_ptr);
extern int  GetNewGroup(int st_gid);
extern int  SetFileGroup(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  ChangeDirGroup(DirEntry *de_ptr);
extern void DisplayDirHelp();
extern void DisplayFileHelp();
extern void ClearHelp();
extern int  GetAvailBytes(std::int64_t *avail_bytes);
extern int  DeleteDirectory(DirEntry *dir_entry);
extern int  ExecuteCommand(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  GetCommandLine(char *command_line);
extern int  GetSearchCommandLine(char *command_line);
extern int  DeleteFile(FileEntry *fe_ptr);
extern int  RemoveFile(FileEntry *fe_ptr);
extern int  RenameDirectory(DirEntry *de_ptr, const std::string& new_name);
extern int  RenameFile(FileEntry *fe_ptr, const std::string& new_name, FileEntry **new_fe_ptr);
extern int  RenameTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  GetRenameParameter(const std::string* old_name, char *new_name);
extern char *CTime(time_t f_time, char *buffer);
extern int  LoginDisk(char *path);
extern int  GetNewLoginPath(char *path);
void PrintSpecialString(WINDOW* win, int y, int x, const std::string& str, int color);
void Print(WINDOW* win, int y, int x, const std::string& str, int color);
extern void PrintOptions(WINDOW *,int, int, const std::string&);
void PrintMenuOptions(
  WINDOW* win,
  int x,
  int y,
  const std::string& str,
  int ncolor,
  int hcolor
);
extern char *FormFilename(char *dest, char *src, unsigned int max_len);
extern char *CutFilename(char *dest, const std::string& src, unsigned int max_len);
std::string CutPathname(const std::string& src, std::size_t max_len);
extern void   Fnsplit(char *path, char *dir, char *name);
std::string MakeExtractCommandLine(
  const std::string& path,
  const std::string& file,
  const std::string& cmd
);
extern int MakeDirectory(DirEntry *father_dir_entry);
extern time_t Mktime(struct tm *tm);
extern int TryInsertArchiveDirEntry(const std::shared_ptr<DirEntry>& tree, char *dir, struct stat *stat);
extern int InsertArchiveFileEntry(const std::shared_ptr<DirEntry>& tree, char *path, struct stat *stat);
extern int MinimizeArchiveTree(const std::shared_ptr<DirEntry>& tree);
extern void HitReturnToContinue();
extern int  BuildFilename(const std::string& in_filename, const char *pattern, char *out_filename);
extern int  ViKey(int ch);
std::optional<CompressMethod> GetFileMethod(const std::string& filename);
extern bool KeyPressed();
extern bool EscapeKeyPressed();
extern int  GetTapeDeviceName();
extern int  MakePath(const std::shared_ptr<DirEntry>& tree, const std::string& dir_path, DirEntry **dest_dir_entry);
extern int  MakeDirEntry(DirEntry *father_dir_entry, const std::string& dir_name);
extern void NormPath(const char *in_path, char *out_path);
extern char *Strtok_r(char *str, const char *delim, char **old);
void ReadProfile(const std::optional<std::filesystem::path>& custom_path);
std::optional<std::string> GetProfileValue(const std::string& key);
double GetDoubleProfileValue(const std::string& key);
int GetIntProfileValue(const std::string& key);
bool GetBooleanProfileValue(const std::string& key);
char GetNumberSeparator();
void ScanSubTree(DirEntry* dir_entry);
extern void GetMaxYX(WINDOW *win, int *height, int *width);
const char* GetHistory();
void InsHistory(const std::string& str);
void ReadHistory(const std::optional<std::filesystem::path>& custom_path);
void SaveHistory();
char* GetMatches(const std::string& base);
extern int  KeyF2Get(DirEntry *start_dir_entry,
               int disp_begin_pos,
               int cursor_pos,
               char *path);
extern void MapF2Window();
extern void UnmapF2Window();
void MvAddStr(int y, int x, const std::string& str);
void MvWAddStr(WINDOW* win, int y, int x, const std::string& str);
void WAddStr(WINDOW* win, const std::string& str);
extern void ClockHandler(int);
std::optional<std::string> GetExtViewer(const std::filesystem::path& filename);
extern void InitClock();
extern void SuspendClock();
std::string ShellQuote(const std::string& src);
int BuildUserFileEntry(
  FileEntry* fe_ptr,
  int max_filename_len,
  int max_linkname_len,
  const std::string& tmpl,
  int linelen,
  char* line
);
int GetUserFileEntryLength(
  int max_filename_len,
  int max_linkname_len,
  const std::string& tmpl
);
std::int64_t AtoLL(const char* cptr);
extern void DisplayTree(WINDOW *win, int start_entry_no, int hilight_no);
extern void ReCreateWindows();
extern int  Getch();

enum class MouseFocus
{
  Dir,
  File,
  Overlay
};

enum class MouseAction
{
  None,
  Ignore,
  Select,
  Activate,
  Tag,
  ScrollUp,
  ScrollDown,
  SwitchToDir,
  SwitchToFile
};

struct MouseEvent
{
  MouseAction action = MouseAction::None;
  int row = 0;
  int col = 0;
};

void EnableMouse();
MouseEvent DecodeMouse(MouseFocus focus);

/* Maps KEY_MOUSE scroll events to KEY_UP/KEY_DOWN; other mouse input to -1. */
int TranslateOverlayMouse(int ch);

extern int  DirUserMode(DirEntry *dir_entry, int ch);
extern int  FileUserMode(FileEntry* file_entry, int ch);
std::optional<std::string> GetUserFileAction(int chkey, int* pchremap);
std::optional<std::string> GetUserDirAction(int chkey, int* pchremap);
bool IsUserActionDefined();
std::optional<std::filesystem::path> Getcwd();
std::filesystem::path GetcwdOrDot();
extern int  RefreshDirWindow();
std::string StrLeft(const char* str, std::size_t count);
std::string FitVisualWidth(std::string_view str, std::size_t width, bool left_justify);
const char* StrVisualIndex(const char* str, std::size_t index);
void TruncateVisual(char* str, std::size_t max_len);
int StrVisualLength(std::string_view str);
void WAttrAddStr(WINDOW* win, int attr, const std::string& str);
void StatOrAbort(const std::string& path, struct stat& st);
std::optional<std::filesystem::path> GetXdgCachePath();
std::optional<std::filesystem::path> GetXdgConfigPath();

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

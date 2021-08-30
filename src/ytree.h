#pragma once

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include "./config.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#if defined(WITH_UTF8)
# include <cwchar>
#endif
#include <memory>
#include <optional>
#include <string>

#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(CURSES_HAVE_NCURSES_NCURSES_H)
# include <ncurses/ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_H)
# include <ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_CURSES_H)
# include <ncurses/curses.h>
#else
# include <curses.h>
#endif

/* Some handy macros... */

#define MINIMUM( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#define MAXIMUM( a, b ) ( ( (a) > (b) ) ? (a) : (b) )


#ifdef WIN32

#define  S_IREAD         S_IRUSR
#define  S_IWRITE        S_IWUSR
#define  S_IEXEC         S_IXUSR

#define  popen           _popen
#define  pclose          _pclose
#define  sys_errlist     _sys_errlist

/* Diese Funktionen koennen direkt umgesetzt werden */
/*--------------------------------------------------*/

#define  echochar( ch )              { addch( ch ); refresh(); }
#define  putp( str )                 puts( str )


/* ... hier ist ein wenig mehr Arbeit noetig ... */
/*-----------------------------------------------*/

#define  vidattr( attr )


/* ... und hier gibt's keine entsprechende Funktion. */
/*---------------------------------------------------*/

#define  typeahead( file )

#endif /* WIN32 */



#ifdef __DJGPP__

/* DJGPP GNU DOS Compiler                           */
/*--------------------------------------------------*/

#define  putp( str )                 puts( str )
#define  vidattr( attr )
#define  typeahead( file )

#endif /* __DJGPP__*/

#ifndef KEY_BTAB
#define KEY_BTAB  0x1d
#endif

#ifndef KEY_END
#define KEY_END   KEY_EOL
#endif

#if defined(_IBMR2) && !defined(_AIX41)
#define echochar( c )  { addch( c ); refresh(); }
#define wgetch( w )    AixWgetch( w )
#endif


#if defined( S_IFLNK ) && !defined( isc386 )
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#define readlink( a, b, c )  (-1)
#endif /* S_IFLNK */

#ifndef S_ISREG
#define S_ISREG( mode )   (((mode) & S_IFMT) == S_IFREG)
#endif /* S_ISREG */

#ifndef S_ISDIR
#define S_ISDIR( mode )   (((mode) & S_IFMT) == S_IFDIR)
#endif /* S_ISDIR */

#ifndef S_ISCHR
#define S_ISCHR( mode )   (((mode) & S_IFMT) == S_IFCHR)
#endif /* S_ISCHR */

#ifndef S_ISBLK
#define S_ISBLK( mode )   (((mode) & S_IFMT) == S_IFBLK)
#endif /* S_ISBLK */

#ifndef S_ISFIFO
#define S_ISFIFO( mode )   (((mode) & S_IFMT) == S_IFIFO)
#endif /* S_ISFIFO */

#ifndef S_ISLNK
#ifdef  S_IFLNK
#define S_ISLNK( mode )   (((mode) & S_IFMT) == S_IFLNK)
#else
#define S_ISLNK( mode )   FALSE
#endif /* S_IFLNK */
#endif /* S_ISLNK */

#ifndef S_ISSOCK
#ifdef  S_IFSOCK
#define S_ISSOCK( mode )   (((mode) & S_IFMT) == S_IFSOCK)
#else
#define S_ISSOCK( mode )   FALSE
#endif /* S_IFSOCK */
#endif /* S_ISSOCK */


#define VI_KEY_UP    'k'
#define VI_KEY_DOWN  'j'
#define VI_KEY_RIGHT 'l'
#define VI_KEY_LEFT  'h'
#define VI_KEY_NPAGE ( 'D' & 0x1F )
#define VI_KEY_PPAGE ( 'U' & 0x1F )


#define OWNER_NAME_MAX  64
#define GROUP_NAME_MAX  64
#define DISPLAY_OWNER_NAME_MAX  12
#define DISPLAY_GROUP_NAME_MAX  12


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


#define PROFILE_FILENAME	".ytree"
#define HISTORY_FILENAME	".ytree-hst"


/* Auswahl der benutzten UNIX-Kommandos */
/*--------------------------------------*/

#define CAT             GetProfileValue( "CAT" )
#define HEXDUMP         GetProfileValue( "HEXDUMP" )
#define EDITOR          GetProfileValue( "EDITOR" )
#define PAGER           GetProfileValue( "PAGER" )
#define MELT            GetProfileValue( "MELT" )
#define UNCOMPRESS      GetProfileValue( "UNCOMPRESS" )
#define GNUUNZIP        GetProfileValue( "GNUUNZIP" )
#define BUNZIP          GetProfileValue( "BUNZIP" )
#define MANROFF         GetProfileValue( "MANROFF" )
#define TARLIST         GetProfileValue( "TARLIST" )
#define TAREXPAND       GetProfileValue( "TAREXPAND" )
#define RPMLIST         GetProfileValue( "RPMLIST" )
#define RPMEXPAND       GetProfileValue( "RPMEXPAND" )
#define ZOOLIST         GetProfileValue( "ZOOLIST" )
#define ZOOEXPAND       GetProfileValue( "ZOOEXPAND" )
#define ZIPLIST         GetProfileValue( "ZIPLIST" )
#define ZIPEXPAND       GetProfileValue( "ZIPEXPAND" )
#define LHALIST         GetProfileValue( "LHALIST" )
#define LHAEXPAND       GetProfileValue( "LHAEXPAND" )
#define ARCLIST         GetProfileValue( "ARCLIST" )
#define ARCEXPAND       GetProfileValue( "ARCEXPAND" )
#define TREEDEPTH       GetProfileValue( "TREEDEPTH" )
#define USERVIEW        GetProfileValue( "USERVIEW" )
#define RARLIST         GetProfileValue( "RARLIST" )
#define RAREXPAND       GetProfileValue( "RAREXPAND" )
#define FILEMODE        GetProfileValue( "FILEMODE" )
#define NUMBERSEP       GetProfileValue( "NUMBERSEP" )
#define NOSMALLWINDOW   GetProfileValue( "NOSMALLWINDOW" )
#define INITIALDIR      GetProfileValue( "INITIALDIR" )
#define DIR1            GetProfileValue( "DIR1" )
#define DIR2            GetProfileValue( "DIR2" )
#define FILE1           GetProfileValue( "FILE1" )
#define FILE2           GetProfileValue( "FILE2" )
#define SEARCHCOMMAND   GetProfileValue( "SEARCHCOMMAND" )
#define HEXEDITOFFSET   GetProfileValue( "HEXEDITOFFSET" )
#define LISTJUMPSEARCH  GetProfileValue( "LISTJUMPSEARCH" )


#define DEFAULT_TREE       "."


#define Error(msg) ErrorEx(msg, __FILE__, __LINE__)
#define ErrorPrintf(format, ...) ErrorPrintfEx(format, __FILE__, __LINE__, __VA_ARGS__)

#define TAGGED_SYMBOL '*'
#define MAX_MODES      11
#define DISK_MODE      0
#define LL_FILE_MODE   1
#define TAR_FILE_MODE  2
#define ZOO_FILE_MODE  3
#define ZIP_FILE_MODE  4
#define LHA_FILE_MODE  5
#define ARC_FILE_MODE  6
#define RPM_FILE_MODE  7
#define RAR_FILE_MODE  8
#define TAPE_MODE      9
#define USER_MODE      10

enum class CompressMethod
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

#define SORT_BY_NAME       1
#define SORT_BY_MOD_TIME   2
#define SORT_BY_CHG_TIME   3
#define SORT_BY_ACC_TIME   4
#define SORT_BY_SIZE       5
#define SORT_BY_OWNER      6
#define SORT_BY_GROUP      7
#define SORT_BY_EXTENSION  8
#define SORT_ASC           10
#define SORT_DSC           20
#define SORT_CASE          40
#define SORT_ICASE         80

#define DEFAULT_FILE_SPEC "*"

#define TAGSYMBOL_VIEWNAME	"tag"
#define FILENAME_VIEWNAME	"fnm"
#define ATTRIBUTE_VIEWNAME	"atr"
#define LINKCOUNT_VIEWNAME	"lct"
#define FILESIZE_VIEWNAME	"fsz"
#define MODTIME_VIEWNAME	"mot"
#define SYMLINK_VIEWNAME	"lnm"
#define UID_VIEWNAME		"uid"
#define GID_VIEWNAME		"gid"
#define INODE_VIEWNAME		"ino"
#define ACCTIME_VIEWNAME	"act"
#define CHGTIME_VIEWNAME	"sct"


#define BLKSIZ             512  /* Blockgroesse fuer SVR3 */

#define CLOCK_INTERVAL	   1

#define FILE_SEPARATOR_CHAR   '/'
#define FILE_SEPARATOR_STRING "/"

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


#define PATH_LENGTH            1024
#define FILE_SPEC_LENGTH       (12 + 1)
#define DISK_NAME_LENGTH       (12 + 1)
#define LL_LINE_LENGTH         512
#define TAR_LINE_LENGTH        512
#define RPM_LINE_LENGTH        512
#define ZOO_LINE_LENGTH        512
#define ZIP_LINE_LENGTH        512
#define LHA_LINE_LENGTH        512
#define ARC_LINE_LENGTH        512
#define RAR_LINE_LENGTH        512
#define MESSAGE_LENGTH         (PATH_LENGTH + 80 + 1)
#define COMMAND_LINE_LENGTH    4096
#define MODE_1                 0
#define MODE_2                 1
#define MODE_3                 2
#define MODE_4                 3
#define MODE_5                 4


#define QUICK_BAUD_RATE      9600

#define ESCAPE               goto FNC_XIT

#define PRINT(ch) (iscntrl(ch) && (((unsigned char)(ch)) < ' ')) ? (ACS_BLOCK) : ((unsigned char)(ch))
/* #define PRINT(ch) (ch) */

#ifdef COLOR_SUPPORT
extern void StartColors(void);
extern void WbkgdSet(WINDOW *w, chtype c);
#else
#define StartColors()	;
#define WbkgdSet(a, b)  ;
#endif /* COLOR_SUPPORT */

struct DirEntry;

struct FileEntry
{
  FileEntry* next;
  FileEntry* prev;
  DirEntry* dir_entry;
  struct stat stat_struct;
  bool tagged;
  bool matching;
  // Symlink name is std::strlen(name) + 1
  char name[1];
};

struct DirEntry
{
  FileEntry* file;
  DirEntry* next;
  DirEntry* prev;
  DirEntry* sub_tree;
  DirEntry* up_tree;
  long long total_bytes;
  long long matching_bytes;
  long long tagged_bytes;
  unsigned int       total_files;
  unsigned int       matching_files;
  unsigned int       tagged_files;
  int                cursor_pos;
  int                start_file;
  struct   stat      stat_struct;
  bool               access_denied;
  bool               global_flag;
  bool               tagged_flag;
  bool               only_tagged;
  bool               not_scanned;
  bool               big_window;
  bool               login_flag;
  char               name[1];
};

struct DirEntryList
{
  unsigned long      indent;
  DirEntry           *dir_entry;
  unsigned short     level;
};

struct FileEntryList
{
  FileEntry          *file;
};

struct Statistic
{
  DirEntry      *tree;
  long long disk_space;
  long long disk_capacity;
  long long disk_total_files;
  long long disk_total_bytes;
  long long disk_matching_files;
  long long disk_matching_bytes;
  long long disk_tagged_files;
  long long disk_tagged_bytes;
  unsigned int  disk_total_directories;
  int           disp_begin_pos;
  int           cursor_pos;
  int           kind_of_sort;
  char          login_path[PATH_LENGTH + 1];
  char          path[PATH_LENGTH + 1];
  char          tape_name[PATH_LENGTH + 1];
  char          file_spec[FILE_SPEC_LENGTH + 1];
  char          disk_name[DISK_NAME_LENGTH + 1];
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
extern int       mode;
extern int       user_umask;
extern bool	 print_time;
extern bool      resize_request;
extern char      number_seperator;
extern bool      bypass_small_window;
extern const char* initial_directory;
extern char 	 builtin_hexdump_cmd[];


extern char *getenv(const char *);

extern int  ytree(int argc, char *argv[]);
extern void DisplayMenu(void);
extern void DisplayDiskStatistic(void);
extern void DisplayDirStatistic(DirEntry *dir_entry);
extern void DisplayDirParameter(DirEntry *dir_entry);
extern void DisplayDirTagged(DirEntry *dir_entry);
extern void DisplayDiskTagged(void);
extern void DisplayDiskName(void);
extern void DisplayFileParameter(FileEntry *file_entry);
extern void DisplayGlobalFileParameter(FileEntry *file_entry);
extern void RefreshWindow(WINDOW *win);
int ReadTree(DirEntry* dir_entry, const std::string& path, int depth);
extern void UnReadTree(DirEntry *dir_entry);
extern int  ReadTreeFromTAR(DirEntry *dir_entry, FILE *f);
extern int  ReadTreeFromRPM(DirEntry *dir_entry, FILE *f);
extern int  ReadTreeFromZOO(DirEntry *dir_entry, FILE *f);
extern int  ReadTreeFromZIP(DirEntry *dir_entry, FILE *f);
extern int  ReadTreeFromLHA(DirEntry *dir_entry, FILE *f);
extern int  ReadTreeFromARC(DirEntry *dir_entry, FILE *f);
extern int  ReadTreeFromRAR(DirEntry *dir_entry, FILE *f);
extern int  GetDiskParameter(char *path,
			     char *volume_name,
			     long long *avail_bytes,
			     long long *capacity
			    );
extern int  HandleDirWindow(DirEntry *start_dir_entry);
extern void DisplayFileWindow(DirEntry *dir_entry);
extern int Init(char *configuration_file, char *history_file);
std::string GetPath(const DirEntry* dir_entry);
extern bool Match(char *file_name);
extern int  SetMatchSpec(char *new_spec);
extern int  SetFileSpec(char *file_spec);
extern void SetMatchingParam(DirEntry *dir_entry);
void ErrorEx(const std::string& msg, const std::string& module, int line);
void ErrorPrintfEx(const char* format, const char* module, int line, ...);
void Warning(const std::string& msg);
void WarningPrintf(const char* format, ...);
void Notice(const std::string& msg);
extern void UnmapNoticeWindow(void);
extern void SetFileMode(int new_file_mode);
extern int  HandleFileWindow(DirEntry *dir_entry);
extern char *GetAttributes(unsigned short modus, char *buffer);
extern void SwitchToSmallFileWindow(void);
extern void SwitchToBigFileWindow(void);
std::optional<std::string> GetGroupName(gid_t gid);
std::optional<int> GetGroupId(const std::string& name);
std::optional<std::string> GetPasswdName(uid_t uid);
std::optional<int> GetPasswdUid(const std::string& name);
std::string GetFileNamePath(const FileEntry* file_entry);
std::string GetRealFileNamePath(const FileEntry* file_entry);
int SystemCall(const std::string& command_line);
int QuerySystemCall(const std::string& command_line);
int SilentSystemCall(const std::string& command_line);
int SilentSystemCallEx(const std::string& command_line, bool enable_clock);
int View(DirEntry* dir_entry, const std::string& file_path);
int ViewHex(const std::string& file_path);
int InternalView(const std::string& file_path);
int Edit(const DirEntry* dir_entry, const std::string& file_path);
extern void DisplayAvailBytes(void);
extern void DisplayFileSpec(void);
extern void QuitTo(DirEntry * dir_entry);
extern void Quit(void);
extern int  ReadFileSpec(void);
extern int  InputString(char *s, int y, int x, int cursor_pos, int length, const char *term);
extern void RotateFileMode(void);
int Execute(const DirEntry* dir_entry, const FileEntry* file_entry);
extern int  Pipe(DirEntry *dir_entry, FileEntry *file_entry);
extern int  PipeTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  GetPipeCommand(char *pipe_command);
extern void GetKindOfSort(void);
extern void SetKindOfSort(int new_kind_of_sort);
extern int  ChangeFileModus(FileEntry *fe_ptr);
extern int  ChangeDirModus(DirEntry *de_ptr);
extern int  GetNewFileModus(int y, int x, char *modus, const char *term);
extern int  GetModus(const char *modus);
extern int  SetFileModus(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  CopyTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  CopyFile(Statistic *statistic_ptr, FileEntry *fe_ptr, unsigned char confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, bool path_copy);
extern int  MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  MoveFile(FileEntry *fe_ptr, unsigned char confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, FileEntry **new_fe_ptr);
extern int  InputChoise(const char *msg, const char *term);
void Message(const std::string& msg);
void MessagePrintf(const char* format, ...);
extern int  GetDirEntry(DirEntry *tree, DirEntry *current_dir_entry, char *dir_path, DirEntry **dir_entry, char *to_path);
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
extern void DisplayDirHelp(void);
extern void DisplayFileHelp(void);
extern void ClearHelp(void);
extern int  GetAvailBytes(long long *avail_bytes);
extern int  DeleteDirectory(DirEntry *dir_entry);
extern int  ExecuteCommand(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  GetCommandLine(char *command_line);
extern int  GetSearchCommandLine(char *command_line);
extern int  DeleteFile(FileEntry *fe_ptr);
extern int  RemoveFile(FileEntry *fe_ptr);
extern int  RenameDirectory(DirEntry *de_ptr, char *new_name);
extern int  RenameFile(FileEntry *fe_ptr, char *new_name, FileEntry **new_fe_ptr);
extern int  RenameTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int  GetRenameParameter(char *old_name, char *new_name);
extern char *CTime(time_t f_time, char *buffer);
extern int  LoginDisk(char *path);
extern int  GetNewLoginPath(char *path);
void PrintSpecialString(WINDOW* win, int y, int x, const std::string& str, int color);
void Print(WINDOW* win, int y, int x, const std::string& str, int color);
extern void PrintOptions(WINDOW *,int, int, const char *);
extern void PrintMenuOptions(WINDOW *,int, int, char *, int, int);
extern char *FormFilename(char *dest, char *src, unsigned int max_len);
extern char *CutFilename(char *dest, char *src, unsigned int max_len);
char* CutPathname(char* dest, const std::string& src, std::size_t max_len);
extern void   Fnsplit(char *path, char *dir, char *name);
void MakeExtractCommandLine(
  char* command_line,
  const std::size_t size,
  const std::string& path,
  const std::string& file,
  const std::string& cmd
);
extern int MakeDirectory(DirEntry *father_dir_entry);
extern time_t Mktime(struct tm *tm);
extern int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat);
extern int InsertArchiveFileEntry(DirEntry *tree, char *path, struct stat *stat);
extern int MinimizeArchiveTree(DirEntry *tree);
extern void HitReturnToContinue(void);
extern int  TermcapWgetch(WINDOW *win);
extern void TermcapVidattr(int attr );
extern void TermcapInitscr(void);
extern void TermcapEndwin(void);
extern int  BuildFilename( char *in_filename, char *pattern, char *out_filename);
extern int  ViKey( int ch );
std::optional<CompressMethod> GetFileMethod(const std::string& filename);
extern int  AixWgetch( WINDOW *w );
extern bool KeyPressed(void);
extern bool EscapeKeyPressed(void);
extern int  GetTapeDeviceName(void);
extern int  MakePath( DirEntry *tree, char *dir_path, DirEntry **dest_dir_entry );
extern int  MakeDirEntry( DirEntry *father_dir_entry, char *dir_name );
extern void NormPath( const char *in_path, char *out_path );
extern char *Strtok_r( char *str, const char *delim, char **old );
extern int  ReadProfile( char *filename );
extern const char *GetProfileValue( const char *key );
void ScanSubTree(DirEntry* dir_entry);
extern void GetMaxYX(WINDOW *win, int *height, int *width);

extern char *GetHistory(void);
extern void InsHistory(char *new_hist);
extern void ReadHistory(char *filename);
extern void SaveHistory(char *filename);
extern char *GetMatches(char *);
extern int  KeyF2Get(DirEntry *start_dir_entry,
               int disp_begin_pos,
               int cursor_pos,
               char *path);
extern void Switch2F2Window(void);
extern void MapF2Window(void);
extern void UnmapF2Window(void);
extern void ReadExtFile(char *);
extern char *GetExtCmd(char *);
void MvAddStr(int y, int x, const std::string& str);
void MvWAddStr(WINDOW* win, int y, int x, const std::string& str);
void WAddStr(WINDOW* win, const std::string& str);
void AddStr(const std::string& str);
extern void ClockHandler(int);
extern int Strrcmp(char *s1, char* s2);
std::optional<std::string> GetExtViewer(const std::string& filename);
extern void InitClock(void);
extern void SuspendClock(void);
std::optional<std::string> GetExtension(const std::string& filename);
std::string ShellEscape(const std::string& src);
extern int  BuildUserFileEntry(FileEntry *fe_ptr,
            int max_filename_len, int max_linkname_len,
            const char *tmpl, int linelen, char *line);
extern int  GetUserFileEntryLength(int max_filename_len,
				   int max_linkname_len, const char *tmpl);
extern long long AtoLL(const char* cptr);
extern void DisplayTree(WINDOW *win, int start_entry_no, int hilight_no);
extern void ReCreateWindows(void);
extern int  Getch(void);
extern int  DirUserMode(DirEntry *dir_entry, int ch);
extern int  FileUserMode(FileEntryList *file_entry_list, int ch);
extern char *GetUserFileAction(int chkey, int *pchremap);
extern char *GetUserDirAction(int chkey, int *pchremap);
extern bool IsUserActionDefined(void);
std::optional<std::string> Getcwd();
std::string GetcwdOrDot();
extern int  RefreshDirWindow();
extern char *StrLeft(const char *str, size_t count);
extern int  StrVisualLength(const char *str);
void WAttrAddStr(WINDOW* win, int attr, const std::string& str);
char* Strdup(const std::string& src);
char* Strndup(const std::string& src, const std::size_t len);
void StatOrAbort(const std::string& path, struct stat& st);

template<class T>
inline T* MallocOrAbort(const std::size_t size)
{
  const auto ptr = static_cast<T*>(std::malloc(size));

  if (!ptr)
  {
    Error("malloc() failed*ABORT");
    std::exit(EXIT_FAILURE);
  }

  return ptr;
}

inline bool Exists(const std::string& path)
{
  return !access(path.c_str(), F_OK);
}

inline bool IsReadable(const std::string& path)
{
  return !access(path.c_str(), R_OK);
}

inline bool IsWriteable(const std::string& path)
{
  return !access(path.c_str(), W_OK);
}

#pragma once

#include "core/api.hpp"
#include "ui/curses.hpp"

#if defined(WITH_UTF8)
# include <cwchar>
#endif

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
extern int  HandleDirWindow(DirEntry *start_dir_entry);
extern void DisplayFileWindow(DirEntry *dir_entry);
void Init(
  const std::optional<std::filesystem::path>& configuration_file,
  const std::optional<std::filesystem::path>& history_file
);
void UnmapNoticeWindow();
extern void SetFileMode(ViewMode new_file_mode);
extern int  HandleFileWindow(DirEntry *dir_entry);
extern void SwitchToSmallFileWindow();
extern void SwitchToBigFileWindow();
bool View(DirEntry* dir_entry, const std::filesystem::path& file_path);
bool ViewHex(const std::filesystem::path& file_path);
bool InternalView(const std::filesystem::path& file_path);
extern void DisplayAvailBytes();
extern void DisplayFileSpec();
std::optional<std::string> InputString(
  const std::string& initial,
  const int y,
  const int x,
  const std::size_t initial_pos,
  const std::size_t max_length
);
extern void RotateFileMode();
std::optional<SortSpec> GetKindOfSort();
extern void DisplayDirHelp();
extern void DisplayFileHelp();
extern void ClearHelp();
void PrintSpecialString(WINDOW* win, int y, int x, const std::string& str, int color);
void Print(WINDOW* win, int y, int x, const std::string& str, int color);
extern void PrintOptions(WINDOW *,int, int, const std::string&);
void PrintMenuOptions(
  WINDOW* win,
  int y,
  int x,
  const std::string& str,
  int ncolor,
  int hcolor
);
extern void HitReturnToContinue();
int ViKey(int ch);
extern bool KeyPressed();
extern bool EscapeKeyPressed();
std::optional<std::string> GetTapeDeviceName();
std::optional<std::string> GetHistory();
void InsHistory(const std::string& str);
void ReadHistory(const std::optional<std::filesystem::path>& custom_path);
void SaveHistory();
std::optional<std::string> GetMatches(const std::string& base);
extern bool KeyF2Get(DirEntry *start_dir_entry,
               int disp_begin_pos,
               int cursor_pos,
               char *path);
extern void MapF2Window();
extern void UnmapF2Window();
void MvAddStr(int y, int x, const std::string& str);
void MvWAddStr(WINDOW* win, int y, int x, const std::string& str);
void WAddStr(WINDOW* win, const std::string& str);
extern void ClockHandler(int);
extern void InitClock();
extern void SuspendClock();
extern void GetMaxYX(WINDOW *win, int *height, int *width);
extern void DisplayTree(WINDOW *win, int start_entry_no, int hilight_no);
extern void ReCreateWindows();
extern int  Getch();
extern bool RefreshDirWindow();
std::string StrLeft(const char* str, std::size_t count);
std::string FitVisualWidth(std::string_view str, std::size_t width, bool left_justify);
const char* StrVisualIndex(const char* str, std::size_t index);
void TruncateVisual(char* str, std::size_t max_len);
int StrVisualLength(std::string_view str);
void WAttrAddStr(WINDOW* win, int attr, const std::string& str);
unsigned char MakePrintable(int ch);

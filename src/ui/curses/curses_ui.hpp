#pragma once

/**
 * Curses-only UI declarations (WINDOW* painting helpers).
 */

#include "ui/curses/curses.hpp"
#include "ui/ui.hpp"

#if defined(WITH_UTF8)
# include <cwchar>
#endif

void RefreshWindow(WINDOW* win);
void PrintSpecialString(WINDOW* win, int y, int x, const std::string& str, int color);
void Print(WINDOW* win, int y, int x, const std::string& str, int color);
void PrintOptions(WINDOW* win, int y, int x, const std::string& str);
void PrintMenuOptions(
  WINDOW* win,
  int y,
  int x,
  const std::string& str,
  int ncolor,
  int hcolor
);
void MvAddStr(int y, int x, const std::string& str);
void MvWAddStr(WINDOW* win, int y, int x, const std::string& str);
void WAddStr(WINDOW* win, const std::string& str);
void WAttrAddStr(WINDOW* win, int attr, const std::string& str);
void GetMaxYX(WINDOW* win, int* height, int* width);
void DisplayTree(WINDOW* win, int start_entry_no, int hilight_no);
void ReCreateWindows();
int Getch();
int ViKey(int ch);
unsigned char MakePrintable(int ch);
const char* StrVisualIndex(const char* str, std::size_t index);
void TruncateVisual(char* str, std::size_t max_len);

std::optional<std::string> InputString(
  const std::string& initial,
  int y,
  int x,
  std::size_t initial_pos,
  std::size_t max_length
);

void Init(
  const std::optional<std::filesystem::path>& configuration_file,
  const std::optional<std::filesystem::path>& history_file
);

void ClockHandler(int);
void InitClock();
void SuspendClock();
void InitClockImpl();
void SuspendClockImpl();

ui::IDirView& NcursesDirViewInstance();
ui::IFileView& NcursesFileViewInstance();
ui::IViewerSurface& NcursesViewerSurfaceInstance();

ui::IDialogs& NcursesDialogsInstance();
ui::ISession& NcursesSessionInstance();
ui::IInput& NcursesInputInstance();
ui::IShellView& NcursesShellViewInstance();

bool KeyF2Get(
  DirEntry* start_dir_entry,
  int disp_begin_pos,
  int cursor_pos,
  char* path
);
void MapF2Window();
void UnmapF2Window();

void DisplayDirHelp();
void DisplayFileHelp();
void DisplayMenu();
void DisplayAvailBytes();
void DisplayFileSpec();
void DisplayDiskName();
void DisplayDiskStatistic();
void DisplayDirStatistic(DirEntry* dir_entry);
void DisplayDirParameter(DirEntry* dir_entry);
void DisplayDirTagged(DirEntry* dir_entry);
void DisplayDiskTagged();
void DisplayFileParameter(FileEntry* file_entry);
void DisplayGlobalFileParameter(FileEntry* file_entry);
void PaintDirStatistic(const ui::DirPanelInfo& info);
void PaintDirParameter(const ui::DirPanelInfo& info);
void PaintDirTagged(const ui::TaggedCounts& counts);
void PaintFileParameter(const ui::FilePanelInfo& info);
void PaintGlobalFileParameter(const ui::FilePanelInfo& info);
void ClearHelp();
void HitReturnToContinue();
void UnmapNoticeWindow();
void SwitchToSmallFileWindow();
void SwitchToBigFileWindow();
std::optional<SortSpec> GetKindOfSort();
std::optional<std::string> GetTapeDeviceName();
std::optional<std::string> GetHistory();
void InsHistory(const std::string& str);
void ReadHistory(const std::optional<std::filesystem::path>& custom_path);
void SaveHistory();
std::optional<std::string> GetMatches(const std::string& base);
bool View(DirEntry* dir_entry, const std::filesystem::path& file_path);
bool InternalView(const std::filesystem::path& file_path);
bool ViewHex(const std::filesystem::path& file_path);

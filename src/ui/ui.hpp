#pragma once

/**
 * Portable UI declarations shared by curses and GTK frontends.
 * Controllers and core include this; WINDOW*-level APIs live in curses headers.
 */

#include "core/api.hpp"
#include "core/ui.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

int HandleDirWindow(DirEntry* start_dir_entry);
int HandleFileWindow(DirEntry* dir_entry);

void MoveEnd(DirEntry** dir_entry);
void MoveHome(DirEntry** dir_entry);

void SetFileMode(ViewMode new_file_mode);
void RotateFileMode();
/** Rebuild the file list for dir_entry and ask the file view to paint it. */
void DisplayFileWindow(DirEntry* dir_entry);

bool RefreshDirWindow();

/** Frontend-provided viewers / prompts (curses implements; GTK may stub). */
bool View(DirEntry* dir_entry, const std::filesystem::path& file_path);
bool ViewHex(const std::filesystem::path& file_path);
std::optional<SortSpec> GetKindOfSort();

/** Current directory under the tree cursor (from the shared dir list). */
DirEntry* CurrentDirListEntry();

struct DirListRow
{
  std::size_t indent = 0;
  std::shared_ptr<DirEntry> dir_entry;
  unsigned short level = 0;
  bool has_next_sibling = false;
};

const std::vector<DirListRow>& DirListRows();
ViewMode GetDirViewMode();
void SetDirWindowMetrics(int height, int width);
int GetDirWindowHeight();
int GetDirWindowWidth();

const std::vector<std::shared_ptr<FileEntry>>& FileListEntries();
ViewMode GetFileViewMode();
int GetFileMaxColumn();
int GetFileWindowHeight();
int GetFileWindowWidth();
int GetFileXStep();
std::size_t GetFileMaxFilenameLen();
std::size_t GetFileMaxLinknameLen();
unsigned GetFileMaxUserviewLen();
bool GetFileReverseSort();
void SetFileHideScroll(int left, int right);
int GetFileHideRight();

struct AppOptions
{
  std::optional<std::filesystem::path> config_file;
  std::optional<std::filesystem::path> history_file;
  std::string initial_dir = ".";
};

AppOptions ParseAppOptions(int argc, char** argv);
std::string ResolveStartupPath(const std::string& path_arg);

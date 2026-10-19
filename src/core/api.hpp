#pragma once

#include "core/domain.hpp"
#include "core/ui.hpp"

bool ReadTree(const std::shared_ptr<DirEntry>& dir_entry, const std::string& path, int depth);
bool GetDiskParameter(
  const std::string& path,
  std::string* volume_name,
  std::int64_t& avail_bytes,
  std::int64_t* capacity
);
std::filesystem::path GetPath(const DirEntry* dir_entry);
bool IsRoot(const std::string& path);
std::shared_ptr<DirEntry> SharedDirEntry(DirEntry* dir_entry);
inline std::weak_ptr<DirEntry> WeakDirEntry(DirEntry* dir_entry)
{
  return SharedDirEntry(dir_entry);
}
bool Match(const std::string& file_name);
bool SetMatchSpec(const std::string& new_spec);
bool SetFileSpec(const std::string& file_spec);
std::string GetAttributes(unsigned short modus);
std::optional<std::string> GetGroupName(int gid);
std::optional<int> GetGroupId(const std::string& name);
std::optional<std::string> GetPasswdName(uid_t uid);
std::optional<int> GetPasswdUid(const std::string& name);
bool SystemCall(const std::string& command_line);
bool QuerySystemCall(const std::string& command_line);
bool SilentSystemCall(const std::string& command_line);
bool Edit(const DirEntry* dir_entry, const std::string& file_path);
bool ViewHex(const std::filesystem::path& file_path);
extern void QuitTo(DirEntry * dir_entry);
void Quit();
std::optional<std::string> ReadFileSpec();
bool Execute(const DirEntry* dir_entry, const FileEntry* file_entry);
std::optional<std::string> GetPipeCommand(const std::string& initial = {});
bool GetNewFileModus(int y, int x, std::string& modus, const char *term);
int GetModus(const std::string& modus);
bool ExecuteCommand(FileEntry* fe_ptr, const std::string& command);
bool CopyFile(
  Statistic* statistic_ptr,
  FileEntry* fe_ptr,
  bool confirm,
  const std::string& to_file,
  DirEntry* dest_dir_entry,
  const std::string& to_dir_path,
  bool path_copy
);
bool MoveFile(
  FileEntry* fe_ptr,
  bool confirm,
  const std::string& to_file,
  DirEntry* dest_dir_entry,
  const std::string& to_dir_path,
  FileEntry** new_fe_ptr
);
int InputChoise(const std::string& msg, const char* term);
bool Confirm(const std::string& message);
int GetDirEntry(
  const std::shared_ptr<DirEntry>& tree,
  DirEntry* current_dir_entry,
  const std::string& dir_path,
  DirEntry** dir_entry,
  std::string& to_path
);
int GetFileEntry(
  DirEntry* de_ptr,
  const std::string& file_name,
  FileEntry** file_entry
);
std::optional<AsToParameter> GetCopyParameter(
  std::optional<std::string_view> from_file,
  bool path_copy
);
std::optional<AsToParameter> GetMoveParameter(
  std::optional<std::string_view> from_file
);
std::optional<std::string> GetNewOwner(int st_uid);
std::optional<std::string> GetNewGroup(int st_gid);
void GetAvailBytes(std::int64_t& avail_bytes);
std::optional<std::string> GetCommandLine(const std::string& initial = {});
std::optional<std::string> GetSearchCommandLine();
extern bool RenameDirectory(DirEntry *de_ptr, const std::string& new_name);
extern bool RenameFile(FileEntry *fe_ptr, const std::string& new_name, FileEntry **new_fe_ptr);
std::optional<std::string> GetRenameParameter(const std::string* old_name);
std::string CTime(std::time_t f_time);

/** Display helpers: unset optional EntryMeta fields render as "-". */
std::string FormatMetaTime(const std::optional<EntryMeta::time_type>& tp);
std::string FormatMetaOwner(const EntryMeta& meta);
std::string FormatMetaGroup(const EntryMeta& meta);
std::string FormatMetaInode(const EntryMeta& meta);
std::string FormatMetaNlink(const EntryMeta& meta);

bool LoginDisk(const std::string& path);
std::optional<std::string> GetNewLoginPath(const std::string& initial);
std::string FormFilename(const std::string& src, std::size_t max_len);
std::string CutFilename(const std::string& src, std::size_t max_len);
std::string CutPathname(const std::string& src, std::size_t max_len);
ArchivePathSplit Fnsplit(std::string path);
std::string MakeExtractCommandLine(
  const std::string& path,
  const std::string& file,
  const std::string& cmd
);
std::optional<std::string> MakeDirectory(DirEntry *father_dir_entry);
extern std::time_t Mktime(std::tm *tm);
bool TryInsertArchiveDirEntry(
  const std::shared_ptr<DirEntry>& tree,
  const char* dir,
  const EntryMeta& meta
);
bool InsertArchiveFileEntry(
  const std::shared_ptr<DirEntry>& tree,
  const std::string& path,
  const EntryMeta& meta,
  std::string symlink_target = {}
);
void MinimizeArchiveTree(const std::shared_ptr<DirEntry>& tree);
void FinalizeArchiveTree(const std::shared_ptr<DirEntry>& dir_entry);

using ArchiveLineFilter = bool (*)(std::string_view line);
using ArchiveMetaParser = std::optional<archive_meta::NamedEntry> (*)(std::string_view line);

bool ReadArchiveListingFilesOnly(
  const std::shared_ptr<DirEntry>& dir_entry,
  FILE* f,
  ArchiveLineFilter is_entry,
  ArchiveMetaParser parse,
  std::string_view unknown_tag
);
bool ReadArchiveListingWithDirs(
  const std::shared_ptr<DirEntry>& dir_entry,
  FILE* f,
  ArchiveMetaParser parse,
  std::string_view unknown_tag
);

bool IsArchiveDirectoryEntry(const std::string& path, const EntryMeta& meta);
std::string BuildFilename(
  const std::string& in_filename,
  const std::string& pattern
);
std::optional<CompressMethod> GetFileMethod(const std::string& filename);
extern bool MakePath(const std::shared_ptr<DirEntry>& tree, const std::string& dir_path, DirEntry **dest_dir_entry);
extern bool MakeDirEntry(DirEntry *father_dir_entry, const std::string& dir_name);
std::string NormPath(const std::string& in_path);

void ReadProfile(const std::optional<std::filesystem::path>& custom_path);
double GetDoubleProfileValue(const std::string& key);
int GetIntProfileValue(const std::string& key);
bool GetBooleanProfileValue(const std::string& key);
char GetNumberSeparator();
void ScanSubTree(DirEntry* dir_entry);
std::optional<std::string> GetExtViewer(const std::filesystem::path& filename);
std::string ShellQuote(const std::string& src);
std::string BuildUserFileEntry(
  const std::shared_ptr<FileEntry>& fe_ptr,
  int max_filename_len,
  int max_linkname_len,
  const std::string& tmpl,
  int linelen
);
int GetUserFileEntryLength(
  int max_filename_len,
  int max_linkname_len,
  const std::string& tmpl
);
std::int64_t AtoLL(std::string_view s);
std::optional<std::string> ReadLine(FILE* f);
extern int  DirUserMode(DirEntry *dir_entry, int ch);
int FileUserMode(const std::shared_ptr<FileEntry>& file_entry, int ch);
ui::InputEvent DirUserMode(DirEntry* dir_entry, const ui::InputEvent& ev);
ui::InputEvent FileUserMode(
  const std::shared_ptr<FileEntry>& file_entry,
  const ui::InputEvent& ev
);
std::optional<std::string> GetUserFileAction(int chkey, int* pchremap);
std::optional<std::string> GetUserDirAction(int chkey, int* pchremap);
bool IsUserActionDefined();
std::optional<std::filesystem::path> Getcwd();
std::filesystem::path GetcwdOrDot();
EntryMeta MetaOrAbort(const std::filesystem::path& path);
std::optional<std::filesystem::path> GetXdgCachePath();
std::optional<std::filesystem::path> GetXdgConfigPath();
std::string StrLeft(const char* str, std::size_t count);
std::string FitVisualWidth(std::string_view str, std::size_t width, bool left_justify);
int StrVisualLength(std::string_view str);
const char* StrVisualIndex(const char* str, std::size_t index);
void TruncateVisual(char* str, std::size_t max_len);

void ErrorEx(const std::string& msg, const std::string& module, int line);
void Warning(const std::string& msg);
void Message(const std::string& msg);
bool KeyPressed();
bool EscapeKeyPressed();

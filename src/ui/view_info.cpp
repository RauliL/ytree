#include "ui/view_info.hpp"

#include "core/api.hpp"

#include <cstring>

ui::DirPanelInfo MakeDirPanelInfo(const DirEntry& dir)
{
  ui::DirPanelInfo info;
  const auto path = GetPath(&dir);
  info.path_line = path.string() + (dir.not_scanned ? "*" : "");
  const auto* p = std::strrchr(
    dir.name.c_str(),
    std::filesystem::path::preferred_separator
  );
  info.name_label = p ? p + 1 : dir.name;
  info.total_files = dir.total_files;
  info.total_bytes = dir.total_bytes;
  info.matching_files = dir.matching_files;
  info.matching_bytes = dir.matching_bytes;
  info.tagged_files = dir.tagged_files;
  info.tagged_bytes = dir.tagged_bytes;
  return info;
}

ui::FilePanelInfo MakeFilePanelInfo(const FileEntry& file)
{
  ui::FilePanelInfo info;
  info.name_label = file.name;
  info.size = file.meta.size;
  return info;
}

ui::FilePanelInfo MakeGlobalFilePanelInfo(const FileEntry& file)
{
  ui::FilePanelInfo info = MakeFilePanelInfo(file);
  info.path_line = GetPath(file.Dir().get()).string();
  return info;
}

ui::TaggedCounts MakeTaggedCounts(const DirEntry& dir)
{
  return ui::TaggedCounts{
    static_cast<std::int64_t>(dir.tagged_files),
    dir.tagged_bytes
  };
}

#pragma once

#include "core/file-entry.hpp"

#include <vector>

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

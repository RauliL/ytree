#pragma once

#include "core/entry.hpp"

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

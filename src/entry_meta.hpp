#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <sys/stat.h>

/**
 * Portable entry metadata shared by filesystem and archive-backed entries.
 *
 * Optional fields are unset when the source does not provide them (typical for
 * archives). Prefer this over embedding struct stat once call sites migrate.
 */
class EntryMeta
{
public:
  using TimePoint = std::chrono::system_clock::time_point;

  std::filesystem::file_type type = std::filesystem::file_type::unknown;

  /** Permission bits only (e.g. 0644); type lives in `type`. */
  std::uint32_t permissions = 0;

  std::int64_t size = 0;

  std::optional<TimePoint> mtime;
  std::optional<TimePoint> atime;
  std::optional<TimePoint> ctime;

  std::optional<std::uint32_t> uid;
  std::optional<std::uint32_t> gid;
  std::optional<std::uint64_t> inode;
  std::optional<std::uint32_t> nlink;

  inline bool IsDir() const
  {
    return type == std::filesystem::file_type::directory;
  }

  inline bool IsLink() const
  {
    return type == std::filesystem::file_type::symlink;
  }

  inline bool IsRegular() const
  {
    return type == std::filesystem::file_type::regular;
  }

  inline bool IsExecutable() const
  {
    return (permissions & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;
  }

  /** Apply a POSIX mode including type bits (as from GetModus / st_mode). */
  void SetFromMode(std::uint32_t mode);

  /** POSIX mode with type bits reconstructed from `type` and `permissions`. */
  std::uint32_t Mode() const;

  /**
   * Read metadata from the real filesystem.
   */
  static std::optional<EntryMeta> FromFilesystem(const std::filesystem::path& path);

  static EntryMeta FromStat(const struct stat& st);

  struct stat ToStat() const;
};

namespace archive_meta
{

struct NamedEntry
{
  std::string name;
  EntryMeta meta;
};

/** Minimal directory metadata for archive tree nodes. */
EntryMeta Directory();

/** Minimal regular-file defaults before a listing parser fills fields. */
EntryMeta RegularFile();

/**
 * Parse one zipinfo listing line into portable metadata.
 * Unknown atime/ctime/owner are left unset (not faked).
 */
std::optional<NamedEntry> ParseZipListing(std::string_view zip_line);

}  // namespace archive_meta

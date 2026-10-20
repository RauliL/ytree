#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

/**
 * Portable entry metadata shared by filesystem and archive-backed entries.
 *
 * Optional fields are unset when the source does not provide them (typical for
 * archives).
 */
class EntryMeta
{
public:
  using size_type = std::int64_t;
  using time_type = std::chrono::system_clock::time_point;

  std::filesystem::file_type type = std::filesystem::file_type::unknown;

  /** Permission bits only; file type lives in `type`. */
  std::filesystem::perms permissions = std::filesystem::perms::none;

  size_type size = 0;

  std::optional<time_type> mtime;
  std::optional<time_type> atime;
  std::optional<time_type> ctime;

  std::optional<std::uint32_t> uid;
  std::optional<std::uint32_t> gid;
  std::optional<std::uint64_t> inode;
  std::optional<std::uint32_t> nlink;

  inline bool IsBlock() const
  {
    return type == std::filesystem::file_type::block;
  }

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
    using std::filesystem::perms;

    return (permissions & (
        perms::owner_exec |
        perms::group_exec |
        perms::others_exec
      )
    ) != perms::none;
  }

  /**
   * Apply a POSIX mode including type bits (as from GetModus / st_mode).
   */
  void SetFromMode(std::uint32_t mode);

  /**
   * Apply a POSIX mode from mode string. (That is passed to `GetModus()`).
   */
  void SetFromMode(std::string_view mode);

  /** POSIX mode with type bits reconstructed from `type` and `permissions`. */
  std::uint32_t Mode() const;

  /** time_t for display/sort; missing optional times map to 0 (epoch). */
  std::time_t MtimeAsTimeT() const;
  std::time_t AtimeAsTimeT() const;
  std::time_t CtimeAsTimeT() const;

  inline std::uint32_t UidOrZero() const
  {
    return uid.value_or(0);
  }

  inline std::uint32_t GidOrZero() const
  {
    return gid.value_or(0);
  }

  /** Single-character type glyph used in file listings. */
  char TypeSymbol() const;

  /** Read metadata from the real filesystem (POSIX lstat/stat today). */
  static std::optional<EntryMeta> FromFilesystem(const std::filesystem::path& path);

  /** As above, with a detailed error code on failure. */
  static std::optional<EntryMeta> FromFilesystem(
    const std::filesystem::path& path,
    std::error_code& ec
  );
};

namespace archive_meta
{

struct NamedEntry
{
  std::string name;
  EntryMeta meta;
  std::string symlink_target;
};

/** Minimal directory metadata for archive tree nodes. */
EntryMeta Directory();

/** Minimal regular-file defaults before a listing parser fills fields. */
EntryMeta RegularFile();

}  // namespace archive_meta

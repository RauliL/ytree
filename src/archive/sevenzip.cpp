#include <cstring>

#include "core/api.hpp"

static constexpr std::string_view kEntriesSeparator = "----------";
static constexpr std::string_view kPathPrefix = "Path = ";
static constexpr std::string_view kSizePrefix = "Size = ";
static constexpr std::string_view kModifiedPrefix = "Modified = ";
static constexpr std::string_view kAttributesPrefix = "Attributes = ";

static inline bool StartsWith(std::string_view line, std::string_view prefix)
{
  return line.size() >= prefix.size() && line.compare(0, prefix.size(), prefix) == 0;
}

static inline std::string_view ValueAfter(std::string_view line, std::string_view prefix)
{
  return line.substr(prefix.size());
}

static bool ParseModified(std::string_view value, std::tm* tm_struct)
{
  // Formats: "YYYY-MM-DD HH:MM:SS" or with fractional seconds.
  if (value.size() < 19)
  {
    return false;
  }

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;

  if (std::sscanf(
        std::string(value).c_str(),
        "%d-%d-%d %d:%d:%d",
        &year,
        &month,
        &day,
        &hour,
        &minute,
        &second
      ) < 6)
  {
    return false;
  }

  std::memset(tm_struct, 0, sizeof(*tm_struct));
  tm_struct->tm_year = year - 1900;
  tm_struct->tm_mon = month - 1;
  tm_struct->tm_mday = day;
  tm_struct->tm_hour = hour;
  tm_struct->tm_min = minute;
  tm_struct->tm_sec = second;
  tm_struct->tm_isdst = -1;

  return true;
}

static void ApplyAttributes(std::string_view attributes, EntryMeta& meta)
{
  // Examples: "A -rw-rw-r--", "D", "D_...."
  const bool is_dir =
    (!attributes.empty() && attributes.front() == 'D') ||
    attributes.find("D_") != std::string_view::npos;

  const auto unix_mode_pos = attributes.find('-');
  if (unix_mode_pos != std::string_view::npos &&
      attributes.size() - unix_mode_pos >= 10)
  {
    meta.SetFromMode(attributes.substr(unix_mode_pos, 10));
    if (is_dir)
    {
      meta.type = std::filesystem::file_type::directory;
    }
    return;
  }

  using std::filesystem::perms;

  if (is_dir)
  {
    meta.type = std::filesystem::file_type::directory;
    meta.permissions = perms::owner_all | perms::group_read | perms::group_exec |
                       perms::others_read | perms::others_exec;
  }
  else
  {
    meta.type = std::filesystem::file_type::regular;
    meta.permissions = perms::owner_read | perms::owner_write |
                       perms::group_read | perms::others_read;
  }
}

static inline EntryMeta ResetEntry()
{
  // Owner/group are not in 7z listings — leave unset.
  return archive_meta::RegularFile();
}

static bool FlushEntry(
  const std::shared_ptr<DirEntry>& dir_entry,
  std::string& path_name,
  EntryMeta& meta,
  bool have_path
)
{
  if (!have_path || path_name.empty())
  {
    return true;
  }

  if (path_name == "." || path_name == "./")
  {
    path_name.clear();
    return true;
  }

  if (IsArchiveDirectoryEntry(path_name, meta))
  {
    if (path_name.back() != std::filesystem::path::preferred_separator)
    {
      path_name.push_back(std::filesystem::path::preferred_separator);
    }
    if (!TryInsertArchiveDirEntry(dir_entry, path_name.data(), meta))
    {
      FormatMessage("unknown 7z dir*{}", path_name);

      return false;
    }
  }
  else if (!InsertArchiveFileEntry(dir_entry, path_name, meta))
  {
    FormatMessage("unknown 7z file*{}", path_name);

    return false;
  }

  path_name.clear();

  return true;
}

bool ReadTreeFrom7ZIP(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  std::string path_name;
  EntryMeta meta = ResetEntry();
  bool in_entries = false;
  bool have_path = false;

  dir_entry->name.clear();

  while (auto line = ReadLine(f))
  {
    if (!in_entries)
    {
      if (*line == kEntriesSeparator)
      {
        in_entries = true;
      }
      continue;
    }

    if (line->empty())
    {
      if (!FlushEntry(dir_entry, path_name, meta, have_path))
      {
        return false;
      }
      have_path = false;
      meta = ResetEntry();
      continue;
    }

    if (StartsWith(*line, kPathPrefix))
    {
      if (!FlushEntry(dir_entry, path_name, meta, have_path))
      {
        return false;
      }
      path_name = std::string(ValueAfter(*line, kPathPrefix));
      have_path = true;
      meta = ResetEntry();
    }
    else if (StartsWith(*line, kSizePrefix))
    {
      const auto value = ValueAfter(*line, kSizePrefix);

      if (!value.empty() && std::isdigit(static_cast<unsigned char>(value.front())))
      {
        meta.size = AtoLL(value);
      }
    }
    else if (StartsWith(*line, kModifiedPrefix))
    {
      std::tm tm_struct{};

      if (ParseModified(ValueAfter(*line, kModifiedPrefix), &tm_struct))
      {
        meta.mtime = std::chrono::system_clock::from_time_t(Mktime(&tm_struct));
      }
    }
    else if (StartsWith(*line, kAttributesPrefix))
    {
      ApplyAttributes(ValueAfter(*line, kAttributesPrefix), meta);
    }
  }

  if (!FlushEntry(dir_entry, path_name, meta, have_path))
  {
    return false;
  }

  FinalizeArchiveTree(dir_entry);

  return true;
}

#include "ytree.h"

#include <string>
#include <string_view>

namespace {

constexpr std::string_view kEntriesSeparator = "----------";
constexpr std::string_view kPathPrefix = "Path = ";
constexpr std::string_view kSizePrefix = "Size = ";
constexpr std::string_view kModifiedPrefix = "Modified = ";
constexpr std::string_view kAttributesPrefix = "Attributes = ";

bool StartsWith(std::string_view line, std::string_view prefix)
{
  return line.size() >= prefix.size() && line.compare(0, prefix.size(), prefix) == 0;
}

std::string_view ValueAfter(std::string_view line, std::string_view prefix)
{
  return line.substr(prefix.size());
}

void TrimTrailingCr(std::string& line)
{
  if (!line.empty() && line.back() == '\r')
  {
    line.pop_back();
  }
}

bool ParseModified(std::string_view value, std::tm* tm_struct)
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

void ApplyAttributes(std::string_view attributes, struct stat* st)
{
  // Examples: "A -rw-rw-r--", "D", "D_...."
  const bool is_dir =
    (!attributes.empty() && attributes.front() == 'D') ||
    attributes.find("D_") != std::string_view::npos;

  const auto unix_mode_pos = attributes.find('-');
  if (unix_mode_pos != std::string_view::npos &&
      attributes.size() - unix_mode_pos >= 10)
  {
    const auto mode = GetModus(std::string(attributes.substr(unix_mode_pos, 10)).c_str());
    st->st_mode = mode;
    if (is_dir)
    {
      st->st_mode = (st->st_mode & ~static_cast<mode_t>(S_IFMT)) | S_IFDIR;
    }
    return;
  }

  if (is_dir)
  {
    st->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  }
  else
  {
    st->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  }
}

bool FlushEntry(
  const std::shared_ptr<DirEntry>& dir_entry,
  std::string& path_name,
  struct stat& st,
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

  if (S_ISDIR(st.st_mode) ||
      (!path_name.empty() &&
       path_name.back() == std::filesystem::path::preferred_separator))
  {
    if (path_name.back() != std::filesystem::path::preferred_separator)
    {
      path_name.push_back(std::filesystem::path::preferred_separator);
    }
    if (TryInsertArchiveDirEntry(dir_entry, path_name.data(), &st))
    {
      FormatMessage("unknown 7z dir*{}", path_name);
      return false;
    }
  }
  else if (InsertArchiveFileEntry(dir_entry, path_name.data(), &st))
  {
    FormatMessage("unknown 7z file*{}", path_name);
    return false;
  }

  path_name.clear();
  return true;
}

} // namespace

int ReadTreeFrom7ZIP(const std::shared_ptr<DirEntry>& dir_entry, FILE* f)
{
  char line_buf[SEVENZIP_LINE_LENGTH + 1];
  std::string path_name;
  struct stat st{};
  bool in_entries = false;
  bool have_path = false;
  bool dir_flag = false;

  dir_entry->name.clear();
  std::memset(&st, 0, sizeof(st));
  st.st_nlink = 1;
  st.st_uid = static_cast<uid_t>(getuid());
  st.st_gid = static_cast<gid_t>(getgid());

  while (std::fgets(line_buf, SEVENZIP_LINE_LENGTH, f) != nullptr)
  {
    std::string line = line_buf;
    if (!line.empty() && line.back() == '\n')
    {
      line.pop_back();
    }
    TrimTrailingCr(line);

    if (!in_entries)
    {
      if (line == kEntriesSeparator)
      {
        in_entries = true;
      }
      continue;
    }

    if (line.empty())
    {
      if (!FlushEntry(dir_entry, path_name, st, have_path))
      {
        return -1;
      }
      have_path = false;
      std::memset(&st, 0, sizeof(st));
      st.st_nlink = 1;
      st.st_uid = static_cast<uid_t>(getuid());
      st.st_gid = static_cast<gid_t>(getgid());
      continue;
    }

    if (StartsWith(line, kPathPrefix))
    {
      if (!FlushEntry(dir_entry, path_name, st, have_path))
      {
        return -1;
      }
      path_name = std::string(ValueAfter(line, kPathPrefix));
      have_path = true;
      std::memset(&st, 0, sizeof(st));
      st.st_nlink = 1;
      st.st_uid = static_cast<uid_t>(getuid());
      st.st_gid = static_cast<gid_t>(getgid());
      st.st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    }
    else if (StartsWith(line, kSizePrefix))
    {
      const auto value = ValueAfter(line, kSizePrefix);
      if (!value.empty() && std::isdigit(static_cast<unsigned char>(value.front())))
      {
        st.st_size = AtoLL(std::string(value).c_str());
      }
    }
    else if (StartsWith(line, kModifiedPrefix))
    {
      std::tm tm_struct{};

      if (ParseModified(ValueAfter(line, kModifiedPrefix), &tm_struct))
      {
        st.st_mtime = Mktime(&tm_struct);
      }
    }
    else if (StartsWith(line, kAttributesPrefix))
    {
      ApplyAttributes(ValueAfter(line, kAttributesPrefix), &st);
    }
  }

  if (!FlushEntry(dir_entry, path_name, st, have_path))
  {
    return -1;
  }

  if (!dir_flag)
  {
    statistic.disk_total_directories++;
    std::memset(&dir_entry->stat_struct, 0, sizeof(struct stat));
    dir_entry->stat_struct.st_mode = S_IFDIR;
  }

  return MinimizeArchiveTree(dir_entry);
}

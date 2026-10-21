#include "core/api.hpp"
#include "core/entry-meta.hpp"
#include "core/stat-macros.hpp"

#include <cerrno>
#include <cstring>

#if defined(S_IFLNK)
# include <unistd.h>
#endif

static bool StatPath(const std::filesystem::path& path, struct stat& st)
{
#if defined(S_IFLNK)
  return !lstat(path.c_str(), &st);
#else
  return !stat(path.c_str(), &st);
#endif
}

static std::filesystem::file_type TypeFromMode(std::uint32_t mode)
{
  if (S_ISREG(mode))
  {
    return std::filesystem::file_type::regular;
  }
  else if (S_ISDIR(mode))
  {
    return std::filesystem::file_type::directory;
  }
  else if (S_ISLNK(mode))
  {
    return std::filesystem::file_type::symlink;
  }
  else if (S_ISCHR(mode))
  {
    return std::filesystem::file_type::character;
  }
  else if (S_ISBLK(mode))
  {
    return std::filesystem::file_type::block;
  }
  else if (S_ISFIFO(mode))
  {
    return std::filesystem::file_type::fifo;
  }
  else if (S_ISSOCK(mode))
  {
    return std::filesystem::file_type::socket;
  }

  return std::filesystem::file_type::unknown;
}

static std::uint32_t TypeToModeBits(std::filesystem::file_type type)
{
  switch (type)
  {
    case std::filesystem::file_type::regular:
      return S_IFREG;

    case std::filesystem::file_type::directory:
      return S_IFDIR;

    case std::filesystem::file_type::symlink:
#if defined(S_IFLNK)
      return S_IFLNK;
#else
      return 0;
#endif

    case std::filesystem::file_type::character:
      return S_IFCHR;

    case std::filesystem::file_type::block:
      return S_IFBLK;

    case std::filesystem::file_type::fifo:
      return S_IFIFO;

    case std::filesystem::file_type::socket:
#if defined(S_IFSOCK)
      return S_IFSOCK;
#else
      return 0;
#endif

    case std::filesystem::file_type::unknown:
    case std::filesystem::file_type::not_found:
    case std::filesystem::file_type::none:
      break;
  }

  return 0;
}

static std::optional<EntryMeta::time_type> TimeFromTimeT(std::time_t t)
{
  if (t == 0)
  {
    return std::nullopt;
  }

  return std::chrono::system_clock::from_time_t(t);
}

static inline std::time_t TimeToTimeT(const std::optional<EntryMeta::time_type>& tp)
{
  return tp ? std::chrono::system_clock::to_time_t(*tp) : 0;
}

void EntryMeta::SetFromMode(std::uint32_t mode)
{
  type = TypeFromMode(mode);
  permissions = static_cast<std::filesystem::perms>(
    mode & static_cast<std::uint32_t>(std::filesystem::perms::mask)
  );
}

void EntryMeta::SetFromMode(std::string_view mode)
{
  SetFromMode(static_cast<std::uint32_t>(GetModus(mode)));
}

std::uint32_t EntryMeta::Mode() const
{
  return TypeToModeBits(type) |
         static_cast<std::uint32_t>(permissions & std::filesystem::perms::mask);
}

std::time_t EntryMeta::MtimeAsTimeT() const
{
  return TimeToTimeT(mtime);
}

std::time_t EntryMeta::AtimeAsTimeT() const
{
  return TimeToTimeT(atime);
}

std::time_t EntryMeta::CtimeAsTimeT() const
{
  return TimeToTimeT(ctime);
}

char EntryMeta::TypeSymbol() const
{
  switch (type)
  {
    case std::filesystem::file_type::symlink:
      return '@';
    case std::filesystem::file_type::socket:
      return '=';
    case std::filesystem::file_type::character:
      return '-';
    case std::filesystem::file_type::block:
      return '+';
    case std::filesystem::file_type::fifo:
      return '|';
    case std::filesystem::file_type::regular:
      return ' ';
    case std::filesystem::file_type::directory:
    case std::filesystem::file_type::unknown:
    case std::filesystem::file_type::none:
    case std::filesystem::file_type::not_found:
      break;
  }
  return '?';
}


static EntryMeta FromStat(const struct stat& st)
{
  EntryMeta meta;

  meta.SetFromMode(static_cast<std::uint32_t>(st.st_mode));
  meta.size = static_cast<std::int64_t>(st.st_size);
  meta.mtime = TimeFromTimeT(st.st_mtime);
  meta.atime = TimeFromTimeT(st.st_atime);
  meta.ctime = TimeFromTimeT(st.st_ctime);
  meta.uid = static_cast<std::uint32_t>(st.st_uid);
  meta.gid = static_cast<std::uint32_t>(st.st_gid);
  meta.inode = static_cast<std::uint64_t>(st.st_ino);
  meta.nlink = static_cast<std::uint32_t>(st.st_nlink);

  return meta;
}

std::optional<EntryMeta> EntryMeta::FromFilesystem(
  const std::filesystem::path& path,
  std::error_code& ec
)
{
  struct stat st{};

  ec.clear();

  if (!StatPath(path, st))
  {
    ec.assign(errno, std::system_category());
    return std::nullopt;
  }

  return FromStat(st);
}

std::optional<EntryMeta> EntryMeta::FromFilesystem(const std::filesystem::path& path)
{
  std::error_code ec;
  return FromFilesystem(path, ec);
}

namespace archive_meta
{

EntryMeta Directory()
{
  using std::filesystem::perms;

  EntryMeta meta;

  meta.type = std::filesystem::file_type::directory;
  meta.permissions = perms::owner_all | perms::group_read | perms::group_exec |
                     perms::others_read | perms::others_exec;
  meta.nlink = 1;

  return meta;
}

EntryMeta RegularFile()
{
  using std::filesystem::perms;

  EntryMeta meta;

  meta.type = std::filesystem::file_type::regular;
  meta.permissions = perms::owner_read | perms::owner_write |
                     perms::group_read | perms::others_read;
  meta.nlink = 1;

  return meta;
}

}  // namespace archive_meta

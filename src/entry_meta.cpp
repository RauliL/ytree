#include "./entry_meta.hpp"

#include <cstring>

#if defined(S_IFLNK)
# include <unistd.h>
#endif

#ifndef S_ISREG
# define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
# define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
# define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
# define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISFIFO
# define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#endif
#ifndef S_ISLNK
# if defined(S_IFLNK)
#  define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
# else
#  define S_ISLNK(mode) (false)
# endif
#endif
#ifndef S_ISSOCK
# if defined(S_IFSOCK)
#  define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
# else
#  define S_ISSOCK(mode) (false)
# endif
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

static std::optional<EntryMeta::TimePoint> TimeFromTimeT(std::time_t t)
{
  if (t == 0)
  {
    return std::nullopt;
  }

  return std::chrono::system_clock::from_time_t(t);
}

static inline std::time_t TimeToTimeT(const std::optional<EntryMeta::TimePoint>& tp)
{
  return tp ? std::chrono::system_clock::to_time_t(*tp) : 0;
}

void EntryMeta::SetFromMode(std::uint32_t mode)
{
  type = TypeFromMode(mode);
  permissions = mode & 07777u;
}

std::uint32_t EntryMeta::Mode() const
{
  return TypeToModeBits(type) | (permissions & 07777u);
}


std::optional<EntryMeta> EntryMeta::FromFilesystem(const std::filesystem::path& path)
{
  struct stat st{};

  if (!StatPath(path, st))
  {
    return std::nullopt;
  }

  return FromStat(st);
}

EntryMeta EntryMeta::FromStat(const struct stat& st)
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

struct stat EntryMeta::ToStat() const
{
  struct stat st{};

  std::memset(&st, 0, sizeof(st));
  st.st_mode = static_cast<mode_t>(Mode());
  st.st_size = static_cast<off_t>(size);
  st.st_mtime = TimeToTimeT(mtime);
  st.st_atime = TimeToTimeT(atime);
  st.st_ctime = TimeToTimeT(ctime);
  st.st_uid = static_cast<uid_t>(uid.value_or(0));
  st.st_gid = static_cast<gid_t>(gid.value_or(0));
  st.st_ino = static_cast<ino_t>(inode.value_or(0));
  st.st_nlink = static_cast<nlink_t>(nlink.value_or(1));

  return st;
}

namespace archive_meta
{

EntryMeta Directory()
{
  EntryMeta meta;

  meta.type = std::filesystem::file_type::directory;
  meta.permissions = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  meta.nlink = 1;

  return meta;
}

EntryMeta RegularFile()
{
  EntryMeta meta;

  meta.type = std::filesystem::file_type::regular;
  meta.permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  meta.nlink = 1;

  return meta;
}

}  // namespace archive_meta

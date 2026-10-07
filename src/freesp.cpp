/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/freesp.c,v 1.25 2012/09/11 16:40:02 werner Exp $
 *
 * Ermittlung der freien Plattenkapazitaet
 *
 ***************************************************************************/


#include "ytree.h"

#if defined(__linux__) || defined(__GNU__)
# include <sys/vfs.h>
#elif defined(__NetBSD__)
# include <sys/statvfs.h>
#else
/* FreeBSD, OpenBSD, macOS, and other modern BSD derivatives */
# include <sys/param.h>
# include <sys/mount.h>
#endif

#ifdef __GNU__
# include <hurd/hurd_types.h>
#endif

#if defined(__NetBSD__)
using FsStat = struct statvfs;
#else
using FsStat = struct statfs;
#endif

static int QueryFs(const char* path, FsStat* buf)
{
#if defined(__NetBSD__)
  return ::statvfs(path, buf);
#else
  return ::statfs(path, buf);
#endif
}

static std::int64_t FsBlockSize(const FsStat& fs)
{
#if defined(__NetBSD__)
  return static_cast<std::int64_t>(fs.f_frsize);
#else
  return static_cast<std::int64_t>(fs.f_bsize);
#endif
}

static const char* FilesystemTypeName(const FsStat& fs)
{
#ifdef __linux__
  switch (fs.f_type)
  {
    case 0xEF51:     return "EXT2-OLD";
    case 0xEF53:     return "EXT2";
    case 0x137D:     return "EXT";
    case 0x9660:     return "ISOFS";
    case 0x137F:     return "MINIX";
    case 0x138F:     return "MINIX2";
    case 0x2468:     return "MINIX-NEW";
    case 0x4d44:     return "DOS";
    case 0x6969:     return "NFS";
    case 0x9fa0:     return "PROC";
    case 0x012FD16D: return "XIAFS";
    default:         return "LINUX";
  }
#elif defined(__GNU__)
  switch (fs.f_type)
  {
    case FSTYPE_UFS:     return "UFS";
    case FSTYPE_NFS:     return "NFS";
    case FSTYPE_GFS:     return "GFS";
    case FSTYPE_LFS:     return "LFS";
    case FSTYPE_SYSV:    return "SYSV";
    case FSTYPE_FTP:     return "FTP";
    case FSTYPE_TAR:     return "TAR";
    case FSTYPE_AR:      return "AR";
    case FSTYPE_CPIO:    return "CPIO";
    case FSTYPE_MSLOSS:  return "DOS";
    case FSTYPE_CPM:     return "CPM";
    case FSTYPE_HFS:     return "HFS";
    case FSTYPE_DTFS:    return "DTFS";
    case FSTYPE_GRFS:    return "GRFS";
    case FSTYPE_TERM:    return "TERM";
    case FSTYPE_DEV:     return "DEV";
    case FSTYPE_PROC:    return "PROC";
    case FSTYPE_IFSOCK:  return "IFSOCK";
    case FSTYPE_AFS:     return "AFS";
    case FSTYPE_DFS:     return "DFS";
    case FSTYPE_PROC9:   return "PROC9";
    case FSTYPE_SOCKET:  return "SOCKET";
    case FSTYPE_MISC:    return "MISC";
    case FSTYPE_EXT2FS:  return "EXT2FS";
    case FSTYPE_HTTP:    return "HTTP";
    case FSTYPE_MEMFS:   return "MEM";
    case FSTYPE_ISO9660: return "ISO9660";
    default:             return "HURD";
  }
#else
  /* FreeBSD, OpenBSD, NetBSD, macOS */
  return fs.f_fstypename;
#endif
}



/* Volume-Name und freien Plattenplatz ermitteln */
/*-----------------------------------------------*/

int GetDiskParameter(const std::string& path,
		      char *volume_name,
		      std::int64_t *avail_bytes,
		      std::int64_t *total_disk_space
)
{
  FsStat fs{};
  char *p;
  const char* fname;
  int  result;
  std::int64_t bfree;
  std::int64_t this_disk_space;

  if ((result = QueryFs(path.c_str(), &fs)) == 0)
  {
    if (volume_name)
    {
      /* Name ermitteln */
      /*----------------*/

      if (mode == Mode::DISK_MODE || mode == Mode::USER_MODE)
      {
        fname = FilesystemTypeName(fs);

        std::strncpy(
          volume_name,
          fname,
          std::min(static_cast<std::size_t>(DISK_NAME_LENGTH), std::strlen(fname))
        );
        volume_name[
          std::min(static_cast<std::size_t>(DISK_NAME_LENGTH), std::strlen(fname))
        ] = '\0';
      }
      else
      {
        /* TAR/ZOO/ZIP-FILE_MODE */
        /*-----------------------*/

        if (!(p = std::strrchr(statistic.login_path, std::filesystem::path::preferred_separator)))
        {
          p = statistic.login_path;
        } else {
          p++;
        }

        std::strncpy(volume_name, p, sizeof(statistic.disk_name));
        volume_name[sizeof(statistic.disk_name)] = '\0';
      }
    } /* volume_name */

    const auto bsize = FsBlockSize(fs);
    bfree = getuid() ? fs.f_bavail : fs.f_bfree;
    if (bfree < 0L)
    {
      bfree = 0L;
    }
    *avail_bytes = bfree * bsize;
    this_disk_space = static_cast<std::int64_t>(fs.f_blocks) * bsize;

    if (total_disk_space)
    {
      *total_disk_space = this_disk_space;
    }
  }
  return result;
}




int GetAvailBytes(std::int64_t *avail_bytes)
{
  return GetDiskParameter(
    statistic.tree->name,
    nullptr,
    avail_bytes,
    nullptr
  );
}

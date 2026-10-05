#pragma once

/* Platform compatibility helpers for POSIX vs Windows builds. */

#if defined(_WIN32) || defined(WIN32)
# ifndef _WIN32
#  define _WIN32 1
# endif
# ifndef WIN32
#  define WIN32 1
# endif
#endif

#if defined(_WIN32)

# include <cstddef>
# include <io.h>
# include <direct.h>
# include <process.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <windows.h>

# if defined(_MSC_VER)
using uid_t = int;
using gid_t = int;
# endif

# ifndef R_OK
#  define R_OK 4
# endif
# ifndef W_OK
#  define W_OK 2
# endif
# ifndef X_OK
#  define X_OK 1
# endif
# ifndef F_OK
#  define F_OK 0
# endif

# ifndef access
#  define access _access
# endif
# ifndef chdir
#  define chdir _chdir
# endif
# ifndef getcwd
#  define getcwd _getcwd
# endif
# ifndef unlink
#  define unlink _unlink
# endif
# ifndef rmdir
#  define rmdir _rmdir
# endif

/* mkdir(path, mode) -> _mkdir(path) */
# ifdef mkdir
#  undef mkdir
# endif
inline int ytree_mkdir(const char* path, int /*mode*/)
{
  return _mkdir(path);
}
# define mkdir ytree_mkdir

inline uid_t getuid()
{
  return 0;
}

inline gid_t getgid()
{
  return 0;
}

inline int umask(int /*mask*/)
{
  return 0;
}

inline int link(const char* /*from*/, const char* /*to*/)
{
  return -1;
}

inline int readlink(const char* /*path*/, char* /*buf*/, size_t /*size*/)
{
  return -1;
}

inline void sleep(unsigned int seconds)
{
  Sleep(seconds * 1000u);
}

# define ERR_TO_NULL " 2>NUL"

#else /* !_WIN32 */

# include <fcntl.h>
# include <grp.h>
# include <pwd.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>

# ifndef ERR_TO_NULL
#  define ERR_TO_NULL " 2> /dev/null"
# endif

#endif /* _WIN32 */

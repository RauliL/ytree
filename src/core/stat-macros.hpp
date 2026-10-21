#pragma once

#include <sys/stat.h>

#if !defined(S_ISREG)
# define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#if !defined(S_ISDIR)
# define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#if !defined(S_ISCHR)
# define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#endif

#if !defined(S_ISBLK)
# define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#endif

#if !defined(S_ISFIFO)
# define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#endif

#if !defined(S_ISLNK)
# if defined(S_IFLNK)
#  define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
# else
#  define S_ISLNK(mode) (false)
# endif
#endif

#if !defined(S_ISSOCK)
# if !defined(S_IFSOCK)
#  define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
# else
#  define S_ISSOCK(mode) (false)
#endif
#endif

#include <sys/mount.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <utime.h>

#ifdef HOST_OS_darwin
#define LIBC_NAME "libSystem.dylib"
#endif

#ifdef HOST_OS_linux
#define LIBC_NAME "libc.so.6"
#endif

#ifndef LIBC_NAME
#define LIBC_NAME "libc.so"
#endif

struct LibCInterface
{
    int		   (*open)(char *path, int oflag, ...);
    int		   (*close)(int filedes);
    int		   (*closedir)(DIR *dirp);
    DIR           *(*opendir)(char *dirname);
    struct dirent *(*readdir)(DIR *dirp);
    void	   (*rewinddir)(DIR *dirp);
    void	   (*seekdir)(DIR *dirp, long loc);
    long	   (*telldir)(DIR *dirp);
    ssize_t        (*read)(int fildes, void *buf, size_t nbyte);
    ssize_t	   (*write)(int fildes, const void *buf, size_t nbyte);
    off_t          (*lseek)(int fildes, off_t offset, int whence);
    int		   (*lstat)(const char *path, struct stat *buf);
    int            (*mkdir)(char *path, mode_t mode);
    int		   (*rmdir)(const char *path);
    int            (*unlink)(const char *path);
    int		   (*link)(char *path1, char *path2);
    int		   (*symlink)(char *path1, char *path2);
    ssize_t	   (*readlink)(char *path, char *buf, size_t bufsize);
    int		   (*rename)(char *old, char *new);
    int		   (*chmod)(char *path, mode_t mode);
    int		   (*ftruncate)(int fildes, off_t length);
    int		   (*isatty)(int fildes);
    int		   (*statfs)(char *path, struct statfs *buf);
    int		   (*utime)(char *path, const struct utimbuf *times);
    struct tm     *(*localtime)(const time_t *clock);
    time_t	   (*mktime)(struct tm *timeptr);
    int		  *(*__error)();
};

struct Emul_PlatformData
{
    void		   *libcHandle;
    struct LibCInterface   *SysIFace;
    int			   *errnoPtr;	/* Pointer to host's errno		 */
    struct SignalSemaphore  sem;	/* Semaphore to single-thread libc calls */
};

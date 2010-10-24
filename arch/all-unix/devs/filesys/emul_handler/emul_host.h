#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>
#include <utime.h>

#ifdef HOST_OS_linux
#include <sys/vfs.h>
#define LIBC_NAME "libc.so.6"
#else
#include <sys/mount.h>
#endif

#ifdef HOST_OS_darwin
#define LIBC_NAME "libSystem.dylib"
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
    char	  *(*getcwd)(char *buf, size_t size);
    char	  *(*getenv)(const char *name);
    struct passwd *(*getpwent)(void);
    void	   (*endpwent)(void);
    int		   (*fcntl)(int fd, int cmd, ...);
    int		   (*select)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
    int		   (*kill)(pid_t pid, int sig);
    int		   (*getpid)(void);
    int		  *(*__error)(void);
#ifdef HOST_OS_linux
    int		   (*__xstat)(int ver, char *path, struct stat *buf);
    int		   (*__lxstat)(int ver, const char *path, struct stat *buf);
    #define stat(path, buf)  __xstat(_STAT_VER, path, buf)
    #define lstat(path, buf) __lxstat(_STAT_VER, path, buf)
#else
    int		   (*stat)(char *path, struct stat *buf);
    int		   (*lstat)(const char *path, struct stat *buf);
#endif
};

struct PlatformHandle
{
    /* Nothing to add here */
};

struct Emul_PlatformData
{
    void		   *libcHandle;
    struct LibCInterface   *SysIFace;
    int			   *errnoPtr;	/* Pointer to host's errno		 */
    int			    my_pid;	/* AROS process ID			 */
    struct SignalSemaphore  sem;	/* Semaphore to single-thread libc calls */
    struct MinList	    readList;	/* Asynchronous read queue		 */
};

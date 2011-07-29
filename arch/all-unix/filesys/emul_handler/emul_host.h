#ifndef RESOURCES_EMUL_HOST_H
#define RESOURCES_EMUL_HOST_H

/* avoid conflicts between our __unused define and the ones that might come in
   via sys/stat.h */
#undef __unused

#ifdef HOST_LONG_ALIGNED
#pragma pack(4)
#endif

#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>
#include <utime.h>

/* Android is not a true Linux ;-) */
#ifdef HOST_OS_android
#undef HOST_OS_linux
#include <sys/vfs.h>
#endif

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

#pragma pack()

struct LibCInterface
{
    int		   (*open)(char *path, int oflag, ...);
    int		   (*close)(int filedes);
    int		   (*closedir)(DIR *dirp);
    DIR           *(*opendir)(char *dirname);
    struct dirent *(*readdir)(DIR *dirp);
    void	   (*rewinddir)(DIR *dirp);
    ssize_t        (*read)(int fildes, void *buf, size_t nbyte);
    ssize_t	   (*write)(int fildes, const void *buf, size_t nbyte);
#ifdef HOST_LONG_ALIGNED
    off_t	   (*lseek)(int fildes, unsigned long offset_l, unsigned long offset_h, int whence);
    int		   (*ftruncate)(int fildes, unsigned long length_l, unsigned long length_h);
#else
    off_t          (*lseek)(int fildes, off_t offset, int whence);
    int		   (*ftruncate)(int fildes, off_t length);
#endif
    int            (*mkdir)(char *path, mode_t mode);
    int		   (*rmdir)(const char *path);
    int            (*unlink)(const char *path);
    int		   (*link)(char *path1, char *path2);
    int		   (*symlink)(char *path1, char *path2);
    ssize_t	   (*readlink)(char *path, char *buf, size_t bufsize);
    int		   (*rename)(char *old, char *new);
    int		   (*chmod)(char *path, mode_t mode);
    int		   (*isatty)(int fildes);
    int		   (*statfs)(char *path, struct statfs *buf);
    int		   (*utime)(char *path, const struct utimbuf *times);
    struct tm     *(*localtime)(const time_t *clock);
    time_t	   (*mktime)(struct tm *timeptr);
    char	  *(*getcwd)(char *buf, size_t size);
    char	  *(*getenv)(const char *name);
    int		   (*fcntl)(int fd, int cmd, ...);
    int		   (*select)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
    int		   (*kill)(pid_t pid, int sig);
    int		   (*getpid)(void);
#ifndef HOST_OS_android
    void	   (*seekdir)(DIR *dirp, long loc);
    long	   (*telldir)(DIR *dirp);
    struct passwd *(*getpwent)(void);
    void	   (*endpwent)(void);
#endif
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

#ifdef HOST_LONG_ALIGNED
/*
 * Somewhat dirty hack to adjust data packing to iOS ARM ABI.
 * Perhaps this can be done in a cleaner and more CPU-abstract way.
 * FIXME: Always assuming little-endian CPU
 */
#define LSeek(fildes, offset, whence) emulbase->pdata.SysIFace->lseek(fildes, (ULONG)offset, (ULONG)((UQUAD)offset >> 32), whence)
#define FTruncate(fildes, length)     emulbase->pdata.SysIFace->ftruncate(fildes, (ULONG)length, (ULONG)((UQUAD)length >> 32))
#else
#define LSeek(fildes, offset, whence) emulbase->pdata.SysIFace->lseek(fildes, offset, whence)
#define FTruncate(fildes, length)     emulbase->pdata.SysIFace->ftruncate(fildes, length)
#endif

struct PlatformHandle
{
    ULONG dirpos; /* Directory search position for Android */
};

struct Emul_PlatformData
{
    void		   *libcHandle;
    struct LibCInterface   *SysIFace;
    int			   *errnoPtr;	/* Pointer to host's errno		 */
    int			    my_pid;	/* AROS process ID			 */
    APTR		    em_UtilityBase;
};

/* Remove this hack later in the ABIv1 development cycle */
#define UtilityBase (emulbase->pdata.em_UtilityBase)

#endif /* RESOURCES_EMUL_HOST_H */

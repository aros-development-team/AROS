#include <exec/libraries.h>
#include <exec/semaphores.h>

#define timeval sys_timeval

#include <sys/types.h>

#undef timeval

#ifdef HOST_OS_linux
#define LIBC_NAME "libc.so.6"
#else
#define LIBC_NAME "libc.so"
#endif

extern int audio_fd;

struct LibCInterface
{
    int	    (*open)(char *path, int oflag, ...);
    int	    (*close)(int filedes);
    int	    (*ioctl)(int d, int request, ...);
    void   *(*mmap)(void *start, size_t length, int prot, int flags, int fd, off_t offset);
    int	    (*munmap)(void *start, size_t length);
    ssize_t (*write)(int fildes, const void *buf, size_t nbyte);
    int	   *(*__error)(void);
};

struct OSS_Base
{
    struct Library	   Lib;
    APTR		   HostLibBase;
    APTR		   LibCHandle;
    struct LibCInterface  *OSSIFace;
    int			  *errnoPtr;
    struct SignalSemaphore sem;
};

#define HostLibBase OSSBase->HostLibBase

#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

/* Android is not a true Linux ;-) */
#ifdef HOST_OS_android
#undef HOST_OS_linux
#endif

#ifdef HOST_OS_linux
#define LIBC_NAME "libc.so.6"
#endif

#ifdef HOST_OS_darwin
#define LIBC_NAME "libSystem.dylib"
#endif

#ifndef LIBC_NAME
#define LIBC_NAME "libc.so"
#endif

/* 
 * On Darwin sigset manipulation functions are redefined as macros, so they are slightly renamed here.
 * However they still present in libc as normal functions.
 */
struct KernelInterface
{
    int     (*raise)(int sig);
    int     (*sigprocmask)(int how, const sigset_t *set, sigset_t *oldset);
    int     (*SigEmptySet)(sigset_t *set);
    int     (*SigFillSet)(sigset_t *set);
    int     (*SigDelSet)(sigset_t *set, int signum);
    int     (*sigsuspend)(const sigset_t *mask);
    int     (*sigaction)(int signum, const struct sigaction *act, struct sigaction *oldact);
    int     (*setitimer)(int which, const struct itimerval *value, struct itimerval *ovalue);
    int     (*mprotect)(const void *addr, size_t len, int prot);
    ssize_t (*write)(int fd, const void *buf, size_t count);
    int    *(*__error)(void);
    void *  (*mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    int     (*munmap)(void *addr, size_t length);
    int     (*getpagesize)(void);
};

struct PlatformData
{
    sigset_t	  sig_int_mask;	/* Mask of signals that Disable() block */
    unsigned int  supervisor;
    int		 *errnoPtr;
};

struct SignalTranslation
{
    short sig;
    short AmigaTrap;
    short CPUTrap;
};

extern struct SignalTranslation sigs[];
extern struct KernelInterface KernelIFace;

void cpu_DispatchContext(struct Task *task, regs_t *regs);

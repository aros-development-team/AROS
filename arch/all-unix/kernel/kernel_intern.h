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
    int     (*sigsuspend)(const sigset_t *mask);
    int     (*sigaction)(int signum, const struct sigaction *act, struct sigaction *oldact);
    int     (*mprotect)(const void *addr, size_t len, int prot);
    ssize_t (*read)(int fd, void *buf, size_t count);
    int	    (*fcntl)(int fd, int cmd, ...);
    void *  (*mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
    int     (*munmap)(void *addr, size_t length);
    int    *(*__error)(void);
#ifdef HOST_OS_android
    int     (*sigwait)(const sigset_t *restrict set, int *restrict sig);
#else
    int     (*SigEmptySet)(sigset_t *set);
    int     (*SigFillSet)(sigset_t *set);
    int     (*SigAddSet)(sigset_t *set, int signum);
    int     (*SigDelSet)(sigset_t *set, int signum);
#endif
};

/*
 * Android's Bionic doesn't have these functions.
 * They are simply inlined in headers.
 */
#ifdef HOST_OS_android
#define SIGEMPTYSET sigemptyset
#define SIGFILLSET  sigfillset
#define SIGADDSET   sigaddset
#define SIGDELSET   sigdelset
#else
#define SIGEMPTYSET(x)   pd->iface->SigEmptySet(x); AROS_HOST_BARRIER
#define SIGFILLSET(x)    pd->iface->SigFillSet(x); AROS_HOST_BARRIER
#define SIGADDSET(x,s)   pd->iface->SigAddSet(x,s); AROS_HOST_BARRIER
#define SIGDELSET(x,s)   pd->iface->SigDelSet(x,s); AROS_HOST_BARRIER
#endif

struct PlatformData
{
    sigset_t		    sig_int_mask;   /* Mask of signals that Disable() block */
    int			   *errnoPtr;
    struct KernelInterface *iface;
};

struct SignalTranslation
{
    short sig;
    short AmigaTrap;
    short CPUTrap;
};

extern struct SignalTranslation const sigs[];

void cpu_DispatchContext(struct Task *task, regs_t *regs, struct PlatformData *pdata);

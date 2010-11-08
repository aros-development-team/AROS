/* On Darwin this definition is required by ucontext.h (which is marked as deprecated) */
#define _XOPEN_SOURCE

#include <ucontext.h>

#ifdef HOST_OS_linux
#define LIBC_NAME "libc.so.6"
#endif

#ifdef HOST_OS_darwin
#define LIBC_NAME "libSystem.dylib"
#endif

#ifndef LIBC_NAME
#define LIBC_NAME "libc.so"
#endif

#define HAVE_PREPAREPLATFORM

struct LibCInterface
{
    int  (*getcontext)(ucontext_t *ucp);
    void (*makecontext)(ucontext_t *ucp, void *func(), int argc, ...);
    int  (*swapcontext)(ucontext_t *oucp, ucontext_t *ucp);
    void (*exit)(int status);
};

struct Exec_PlatformData
{
    struct LibCInterface *SysIFace;
    void (*Reboot)(unsigned char warm);
};

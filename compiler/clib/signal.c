#include <stdio.h>
#include <errno.h>
#include <signal.h>

__sighandler_t *signal(int sig, __sighandler_t *handler)
{
    GETUSER;
#warning implement signal

    fprintf(stderr, "**clib warning**: signal() not implemented\n");
    errno = ENOSYS;
    return (__sighandler_t *)-1;
}

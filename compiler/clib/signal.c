#include <stdio.h>
#include <errno.h>
#include <signal.h>

__sighandler_t *signal(int sig, __sighandler_t *handler)
{
#warning implement signal
    errno = ENOSYS;
    return (__sighandler_t *)-1;
}

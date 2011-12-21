#include <fcntl.h>
#include <stdio.h>

#include "log.h"

/*
 * Under MinGW32CE we have problems with dup2() because STDERR_FILENO is actually fileno(stderr),
 * which is, in its turn, raw HANDLE. In order to replace it, we reassign stderr.
 * Under desktop Windows this is also going to work.
 */
int SetLog(const char *c)
{
    return (freopen(c, "ra", stderr) == NULL) ? -1 : 0;
}

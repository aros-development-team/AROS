#include <fcntl.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <runtime.h>

#include "log.h"

int SetLog(const char *c)
{
    /*
     * Redirect stderr on filedescriptor level.
     * Redirecting on streams level does not work with Android's Bionic
     */
    int fd = open(c, O_WRONLY|O_CREAT|O_APPEND, 0644);

    if (fd == -1)
    {
        DisplayError("Failed to redirect debug output to %s", c);
        return -1;
    }
    dup2(fd, STDERR_FILENO);
    fprintf(stderr, "----\n");

    return 0;
}

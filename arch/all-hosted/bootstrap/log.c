#include <fcntl.h>
#include <unistd.h>

#include "log.h"

int SetLog(const char *c)
{
    /*
     * Redirect stderr on filedescriptor level.
     * Redirecting on streams level does not work with Android's Bionic
     */
    int fd = open(c, O_WRONLY|O_CREAT|O_APPEND, 0644);

    if (fd == -1)
        return -1;

    if (dup2(fd, STDERR_FILENO) == -1)
    {
        close(fd);
        return -1;
    }

    return 0;
}

#include <errno.h>
#include <utime.h>

int utime(const char *filename, struct utimbuf *buf)
{
    errno = EACCES;
    return -1;
}

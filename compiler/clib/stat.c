#include <errno.h>
#include <sys/stat.h>

int stat(const char *path, struct stat *sb)
{
    errno = EIO;
    return -1;
}
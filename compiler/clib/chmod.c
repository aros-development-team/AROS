#include <errno.h>
#include <sys/stat.h>

int chmod(const char *path, mode_t mode)
{
    errno = EACCES;
    return -1;
}
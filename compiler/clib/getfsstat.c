#include <errno.h>
#include <sys/mount.h>


int getfsstat(struct statfs *buf, long bufsize, int flags)
{
#warning implement getfsstat

    errno = ENOSYS;
    return -1;
}

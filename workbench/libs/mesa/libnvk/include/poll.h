/* <poll.h> for mesa's libsync.h on AROS. Sync files do not exist here, so
   the poll on one never happens; the declaration lets the header parse. */
#ifndef _NVK_POLL_H
#define _NVK_POLL_H
#include <errno.h>
#define POLLIN   0x001
#define POLLOUT  0x004
#define POLLERR  0x008
#define POLLHUP  0x010
#define POLLNVAL 0x020
struct pollfd {
    int fd;
    short events;
    short revents;
};
typedef unsigned long nfds_t;
static inline int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    (void)fds; (void)nfds; (void)timeout;
    errno = ENOSYS;
    return -1;
}
#endif

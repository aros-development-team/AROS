/*
 * AROS stub for poll.h, which Mesa's util/libsync.h needs for sync_wait().
 * There are no fence fds here - jobs are waited on through the v3d shim - so
 * the call is never reached; ENOSYS guards a caller that does.
 */
#ifndef POLL_H_AROS
#define POLL_H_AROS

#include <errno.h>

#define POLLIN      0x0001
#define POLLPRI     0x0002
#define POLLOUT     0x0004
#define POLLERR     0x0008
#define POLLHUP     0x0010
#define POLLNVAL    0x0020

typedef unsigned long nfds_t;

struct pollfd
{
    int   fd;
    short events;
    short revents;
};

static inline int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    errno = ENOSYS;
    return -1;
}

#endif

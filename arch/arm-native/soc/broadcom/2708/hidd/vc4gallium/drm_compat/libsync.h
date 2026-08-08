/*
    AROS stub for libsync.h
    Sync file fence operations — we don't support native fences,
    so these are stubs that fail gracefully.
*/
#ifndef _LIBSYNC_H_AROS_
#define _LIBSYNC_H_AROS_

/* sync_wait: wait for sync file fd. Returns 0 on success, -1 on failure. */
static inline int sync_wait(int fd, int timeout)
{
    (void)fd;
    (void)timeout;
    return -1;
}

/* sync_accumulate: merge sync fd into *fd_out. */
static inline int sync_accumulate(const char *name, int *fd_out, int fd)
{
    (void)name;
    (void)fd_out;
    (void)fd;
    return -1;
}

#endif

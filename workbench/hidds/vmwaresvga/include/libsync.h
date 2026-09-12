/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    There are no sync file descriptors on AROS; every fence fd the svga
    winsys sees is -1 and these never get called with anything else.
*/
#ifndef _LIBSYNC_H_
#define _LIBSYNC_H_
#include <errno.h>
static inline int sync_wait(int fd, int timeout) { return -1; }
static inline int sync_merge(const char *name, int fd1, int fd2) { return -1; }
static inline int sync_accumulate(const char *name, int *fd1, int fd2) { return -1; }
#endif

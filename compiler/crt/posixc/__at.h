#ifndef ___AT_H
#define ___AT_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: helpers for the POSIX.1-2008 *at() functions (openat, fstatat, ...)
*/

#include <dos/dos.h>

/* Make the directory referenced by dirfd the current directory for the
   duration of an *at() call. dirfd may be AT_FDCWD (no change). On success
   returns 0 and the caller must later call __at_leave() with the two
   values; on failure returns -1 with errno set. */
int __at_enter(int dirfd, BPTR *oldcd, BPTR *dirlock);
void __at_leave(BPTR oldcd, BPTR dirlock);

/* Non-zero when pathname does not need to be resolved against dirfd. */
int __at_isabsolute(const char *pathname);

#endif /* ___AT_H */

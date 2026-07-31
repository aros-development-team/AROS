#ifndef FD_LIBRARY_H
#define FD_LIBRARY_H

/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef UBYTE fd_owner_t;

#define FD_OWNER_NONE      ((fd_owner_t)0)
#define FD_OWNER_POSIXC    ((fd_owner_t)1)
#define FD_OWNER_BSDSOCKET ((fd_owner_t)2)
#define FD_OWNER_MAX       ((fd_owner_t)16)   /* size of the owner-hooks table */

/*
    Per-owner operation hooks.  Each owner (posixc.library for filesystem
    descriptors, bsdsocket.library for network descriptors) registers one
    table describing how to act on the descriptors it creates.  A consumer
    such as posixc's read()/write() then treats every descriptor uniformly:
    it looks up the owning hooks and calls through them, passing the owner
    data (FD_GetData()) as the object to act on.

    Each hook reports errors by writing an errno-style code through perror
    (left untouched on success) and returning -1 (or 0/NULL as appropriate).
    A NULL hook pointer means "operation not supported"; the caller then
    substitutes a sensible errno (e.g. ESPIPE/ENOTTY).
*/
struct fd_hooks
{
    ULONG   fdh_Version;                                             /* FD_HOOKS_VERSION */
    SIPTR   (*fdh_read) (APTR data, APTR buf, IPTR nbytes, LONG *perror);
    SIPTR   (*fdh_write)(APTR data, CONST_APTR buf, IPTR nbytes, LONG *perror);
    /* fd is the specific descriptor being closed - a single object (socket)
       may be referenced by several descriptors (dup()), so the owner must
       release exactly this one, not merely one that maps to the object. */
    LONG    (*fdh_close)(APTR data, LONG fd, LONG *perror);
    QUAD    (*fdh_lseek)(APTR data, QUAD offset, LONG whence, LONG *perror);
    LONG    (*fdh_ioctl)(APTR data, IPTR request, APTR arg, LONG *perror);
    LONG    (*fdh_fcntl)(APTR data, LONG cmd, IPTR arg, LONG *perror);
    LONG    (*fdh_fstat)(APTR data, APTR statbuf, LONG *perror);
    /* Duplicate the descriptor so that newfd refers to the same object.
       The hook reserves newfd with its owner in fd.library.  Returns 0 on
       success, -1 (with *perror) on failure. */
    LONG    (*fdh_dup)(APTR data, LONG newfd, LONG *perror);
};

#define FD_HOOKS_VERSION 1

LONG FD_Alloc(LONG startfd, fd_owner_t owner, APTR data, LONG *outfd);
LONG FD_Reserve(LONG fd, fd_owner_t owner, APTR data);
LONG FD_Free(LONG fd, fd_owner_t owner);
LONG FD_Check(LONG fd);
fd_owner_t FD_GetOwner(LONG fd);
APTR FD_GetData(LONG fd);
LONG FD_SetData(LONG fd, fd_owner_t owner, APTR data);
LONG FD_SetOwnerHooks(fd_owner_t owner, const struct fd_hooks *hooks);
const struct fd_hooks *FD_GetOwnerHooks(fd_owner_t owner);

#ifdef __cplusplus
}
#endif

#endif /* FD_LIBRARY_H */

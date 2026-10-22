#ifndef FD_LIBRARY_H
#define FD_LIBRARY_H

/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
*/

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef UBYTE fd_owner_t;

#define FD_OWNER_NONE      ((fd_owner_t)0)
#define FD_OWNER_POSIXC    ((fd_owner_t)1)
#define FD_OWNER_BSDSOCKET ((fd_owner_t)2)
#define FD_OWNER_NATIVE_SOCKET ((fd_owner_t)3) /* task-independent sockets */
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

    Every hook also gets the descriptor, fd. The caller read data without a
    lock, so another task may have closed fd and freed the object meanwhile;
    the owner checks fd still leads to data, under its own lock, before it
    touches the object.
*/
struct fd_hooks
{
    ULONG   fdh_Version;                                             /* FD_HOOKS_VERSION */
    SIPTR   (*fdh_read) (APTR data, LONG fd, APTR buf, IPTR nbytes, LONG *perror);
    SIPTR   (*fdh_write)(APTR data, LONG fd, CONST_APTR buf, IPTR nbytes, LONG *perror);
    /* fd is the specific descriptor being closed - a single object (socket)
       may be referenced by several descriptors (dup()), so the owner must
       release exactly this one, not merely one that maps to the object. */
    LONG    (*fdh_close)(APTR data, LONG fd, LONG *perror);
    QUAD    (*fdh_lseek)(APTR data, LONG fd, QUAD offset, LONG whence, LONG *perror);
    LONG    (*fdh_ioctl)(APTR data, LONG fd, IPTR request, APTR arg, LONG *perror);
    LONG    (*fdh_fcntl)(APTR data, LONG fd, LONG cmd, IPTR arg, LONG *perror);
    LONG    (*fdh_fstat)(APTR data, LONG fd, APTR statbuf, LONG *perror);
    /* Duplicate the descriptor so that newfd refers to the same object.
       The hook reserves newfd with its owner in fd.library.  Returns 0 on
       success, -1 (with *perror) on failure. */
    LONG    (*fdh_dup)(APTR data, LONG fd, LONG newfd, LONG *perror);
};

#define FD_HOOKS_VERSION 1

/* An operation lease is zero-initialized, non-copyable and owned by one
   caller. Keep fd.library open until release. Its binding is coherent and
   cannot be changed by FD_SetData/FD_Free while held. Providers must use
   FD_Retire and keep their object alive until the retirement callback;
   this API cannot retain an object a provider destroys independently.
   Hook withdrawal/replacement returns EBUSY until every lease and retirement
   callback has completed. Providers must honor that result before unloading. */
struct fd_lease
{
    fd_owner_t owner;
    const struct fd_hooks *hooks;
    APTR data;
    APTR private_record;
};
typedef LONG (*fd_retire_fn)(APTR data, APTR cookie);

LONG FD_Acquire(LONG fd, struct fd_lease *lease);
/* Returns the finalizer result when this release performs retirement,
   otherwise zero. An ordinary operation must preserve its own result/errno. */
LONG FD_Release(struct fd_lease *lease);
/* Withdraw only the exact reservation held by lease. The number is immediately
   reusable. Callback runs once, outside registry exclusion, on the final
   release (possibly in another task). No callback runs inside FD_Retire.
   The provider owns cancellation, close error reporting and callback context. */
LONG FD_Retire(struct fd_lease *lease, fd_retire_fn retire, APTR cookie);
/* Provider-side withdrawal requires no allocation or new lease. The provider
   must hold its own object reference and serialize its descriptor table while
   matching fd/owner/data; generic consumers must use lease identity instead.
   On success the reservation is detached. With no admitted users, the callback
   runs before return and its result is written to close_error. Otherwise the
   callback is deferred and close_error is zero. The output pointer is never
   retained. A nonzero function result means nothing was detached; it is
   separate from a completed close whose callback reported an error. */
LONG FD_RetireOwner(LONG fd, fd_owner_t owner, APTR data,
                   fd_retire_fn retire, APTR cookie, LONG *close_error);

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

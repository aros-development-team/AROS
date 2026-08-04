/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Per-owner operation hooks.  An owner (posixc.library for filesystem
    descriptors, bsdsocket.library for network descriptors) registers one
    table of operations for the descriptors it creates.  A consumer resolves
    a descriptor's owner with FD_GetOwner() and its hooks with
    FD_GetOwnerHooks(), then acts on the descriptor uniformly regardless of
    whether it is a file or a socket.
*/

#include <aros/libcall.h>
#include <proto/exec.h>
#include <errno.h>

#include "fd_private.h"
#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, FD_SetOwnerHooks,

/*  SYNOPSIS */
        AROS_LHA(fd_owner_t, owner, D0),
        AROS_LHA(const struct fd_hooks *, hooks, A0),

/*  LOCATION */
        struct fd_base *, FDBase, 12, FD)

/*  FUNCTION
        Register (or clear) the operation hooks for an owner.  The hooks
        describe how read()/write()/close()/lseek()/ioctl()/fcntl()/fstat()
        act on descriptors created by that owner.

    INPUTS
        owner - Descriptor owner identifier (FD_OWNER_POSIXC, ...).
        hooks - Pointer to a persistent struct fd_hooks, or NULL to clear.

    RESULT
        0 on success, or an errno-style error code.

    NOTES
        The hooks table must remain valid for as long as descriptors owned by
        this owner may be accessed; it is stored by reference, not copied.

    SEE ALSO
        FD_GetOwnerHooks()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (owner == FD_OWNER_NONE || owner >= FD_OWNER_MAX)
        return EINVAL;

    ObtainSemaphore(&FDBase->fd_Lock);
    FDBase->fd_OwnerHooks[owner] = hooks;
    ReleaseSemaphore(&FDBase->fd_Lock);

    return 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(const struct fd_hooks *, FD_GetOwnerHooks,

/*  SYNOPSIS */
        AROS_LHA(fd_owner_t, owner, D0),

/*  LOCATION */
        struct fd_base *, FDBase, 13, FD)

/*  FUNCTION
        Return the operation hooks registered for an owner.

    INPUTS
        owner - Descriptor owner identifier.

    RESULT
        Pointer to the owner's struct fd_hooks, or NULL if none is registered.

    NOTES

    SEE ALSO
        FD_SetOwnerHooks(), FD_GetOwner()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct fd_hooks *hooks = NULL;

    if (owner != FD_OWNER_NONE && owner < FD_OWNER_MAX)
    {
        ObtainSemaphore(&FDBase->fd_Lock);
        hooks = FDBase->fd_OwnerHooks[owner];
        ReleaseSemaphore(&FDBase->fd_Lock);
    }

    return hooks;

    AROS_LIBFUNC_EXIT
}

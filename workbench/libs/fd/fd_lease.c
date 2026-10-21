/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Distributed under the AROS Public License 1.1.

    Coherent descriptor admission and deferred reservation retirement.
*/

#include <aros/libcall.h>
#include <proto/exec.h>
#include <errno.h>
#include <limits.h>

#include "fd_private.h"
#include LC_LIBDEFS_FILE

struct fd_retention
{
    LONG fd;
    fd_owner_t owner;
    APTR data;
    const struct fd_hooks *hooks;
    ULONG users;
    BOOL retired;
    fd_retire_fn retire;
    APTR cookie;
};

AROS_LH2(LONG, FD_Acquire,
    AROS_LHA(LONG, fd, D0),
    AROS_LHA(struct fd_lease *, lease, A0),
    struct fd_base *, FDBase, 14, FD)
{
    AROS_LIBFUNC_INIT
    LONG error = 0;
    fd_entry *entry;
    struct fd_retention *record;

    if (!lease || lease->private_record)
        return EINVAL;
    lease->owner = FD_OWNER_NONE;
    lease->hooks = NULL;
    lease->data = NULL;

    ObtainSemaphore(&FDBase->fd_Lock);
    if (fd < 0 || (ULONG)fd >= FDBase->fd_Slots) {
        error = EBADF;
        goto done;
    }
    entry = &FDBase->fd_Table[fd];
    if (entry->owner == FD_OWNER_NONE || entry->owner >= FD_OWNER_MAX ||
        !entry->data || !FDBase->fd_OwnerHooks[entry->owner]) {
        error = EBADF;
        goto done;
    }
    record = entry->retention;
    if (!record) {
        if (FDBase->fd_OwnerUsers[entry->owner] == (ULONG)~0U) {
            error = EOVERFLOW;
            goto done;
        }
        record = AllocVec(sizeof(*record), MEMF_ANY | MEMF_CLEAR);
        if (!record) {
            error = ENOMEM;
            goto done;
        }
        record->fd = fd;
        record->owner = entry->owner;
        record->data = entry->data;
        record->hooks = FDBase->fd_OwnerHooks[entry->owner];
        entry->retention = record;
        FDBase->fd_OwnerUsers[record->owner]++;
    }
    if (record->users == (ULONG)~0U) {
        error = EOVERFLOW;
        goto done;
    }
    record->users++;
    lease->owner = record->owner;
    lease->hooks = record->hooks;
    lease->data = record->data;
    lease->private_record = record;
done:
    ReleaseSemaphore(&FDBase->fd_Lock);
    return error;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, FD_Release,
    AROS_LHA(struct fd_lease *, lease, A0),
    struct fd_base *, FDBase, 15, FD)
{
    AROS_LIBFUNC_INIT
    struct fd_retention *record;
    BOOL last;
    LONG result = 0;

    if (!lease || !lease->private_record)
        return 0;
    ObtainSemaphore(&FDBase->fd_Lock);
    record = lease->private_record;
    lease->private_record = NULL;
    lease->owner = FD_OWNER_NONE;
    lease->hooks = NULL;
    lease->data = NULL;
    last = --record->users == 0;
    if (last && !record->retired)
        FDBase->fd_Table[record->fd].retention = NULL;
    ReleaseSemaphore(&FDBase->fd_Lock);

    if (last) {
        /* Keep provider admission pinned through arbitrary callback code.
           No provider callback may run with the registry lock held. */
        if (record->retired)
            result = record->retire(record->data, record->cookie);
        ObtainSemaphore(&FDBase->fd_Lock);
        FDBase->fd_OwnerUsers[record->owner]--;
        ReleaseSemaphore(&FDBase->fd_Lock);
        FreeVec(record);
    }
    return result;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(LONG, FD_Retire,
    AROS_LHA(struct fd_lease *, lease, A0),
    AROS_LHA(fd_retire_fn, retire, A1),
    AROS_LHA(APTR, cookie, A2),
    struct fd_base *, FDBase, 16, FD)
{
    AROS_LIBFUNC_INIT
    LONG error = 0;
    struct fd_retention *record;
    fd_entry *entry;

    if (!lease || !lease->private_record || !retire)
        return EINVAL;
    ObtainSemaphore(&FDBase->fd_Lock);
    record = lease->private_record;
    entry = &FDBase->fd_Table[record->fd];
    if (record->retired || entry->retention != record) {
        error = EBADF;
        goto done;
    }
    record->retired = TRUE;
    record->retire = retire;
    record->cookie = cookie;
    entry->owner = FD_OWNER_NONE;
    entry->data = NULL;
    entry->retention = NULL;
done:
    ReleaseSemaphore(&FDBase->fd_Lock);
    return error;
    AROS_LIBFUNC_EXIT
}

AROS_LH6(LONG, FD_RetireOwner,
    AROS_LHA(LONG, fd, D0),
    AROS_LHA(fd_owner_t, owner, D1),
    AROS_LHA(APTR, data, A0),
    AROS_LHA(fd_retire_fn, retire, A1),
    AROS_LHA(APTR, cookie, A2),
    AROS_LHA(LONG *, close_error, A3),
    struct fd_base *, FDBase, 17, FD)
{
    AROS_LIBFUNC_INIT
    LONG error = 0;
    fd_entry *entry;
    BOOL immediate = FALSE;

    if (!close_error)
        return EINVAL;
    *close_error = 0;
    if (!retire || !data || owner == FD_OWNER_NONE || owner >= FD_OWNER_MAX)
        return EINVAL;

    ObtainSemaphore(&FDBase->fd_Lock);
    if (fd < 0 || (ULONG)fd >= FDBase->fd_Slots) {
        error = EBADF;
        goto done;
    }
    entry = &FDBase->fd_Table[fd];
    if (entry->owner != owner || entry->data != data) {
        error = EBADF;
        goto done;
    }
    if (entry->retention) {
        struct fd_retention *record = entry->retention;
        record->retired = TRUE;
        record->retire = retire;
        record->cookie = cookie;
    } else {
        /* No allocation is needed to close an idle descriptor. Pin the
           provider through the immediate callback, just as Release does. */
        if (FDBase->fd_OwnerUsers[owner] == (ULONG)~0U) {
            error = EOVERFLOW;
            goto done;
        }
        FDBase->fd_OwnerUsers[owner]++;
        immediate = TRUE;
    }
    entry->owner = FD_OWNER_NONE;
    entry->data = NULL;
    entry->retention = NULL;
done:
    ReleaseSemaphore(&FDBase->fd_Lock);
    if (immediate) {
        *close_error = retire(data, cookie);
        ObtainSemaphore(&FDBase->fd_Lock);
        FDBase->fd_OwnerUsers[owner]--;
        ReleaseSemaphore(&FDBase->fd_Lock);
    }
    return error;
    AROS_LIBFUNC_EXIT
}

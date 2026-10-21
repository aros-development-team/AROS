#ifndef FD_PRIVATE_H
#define FD_PRIVATE_H

/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>

#include <libraries/fd.h>

struct fd_retention;

typedef struct fd_entry {
    fd_owner_t owner;
    APTR data;
    struct fd_retention *retention;
} fd_entry;

struct fd_base {
    struct Library fd_LibNode;
    struct SignalSemaphore fd_Lock;
    fd_entry *fd_Table;
    ULONG fd_Slots;
    const struct fd_hooks *fd_OwnerHooks[FD_OWNER_MAX];
    ULONG fd_OwnerUsers[FD_OWNER_MAX];
};

#endif /* FD_PRIVATE_H */

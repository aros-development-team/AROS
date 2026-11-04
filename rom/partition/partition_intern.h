#ifndef PARTITION_INTERN_H
#define PARTITION_INTERN_H

/*
    Copyright © 2001-2026, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Internal definitions for partition.library
   Lang: english
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <libraries/partition.h>

#include <aros/libcall.h>

struct PartitionBase_intern
{
    struct PartitionBase partbase;
    BPTR segList;
    struct List bootList;
    struct SignalSemaphore bootSem;

    /* We do NOT autoinit DOSBase, because we want to be
     * explicit about all uses of DOSBase, since it may
     * be NULL (and that's ok!).
     */
    struct Library *pb_DOSBase;

    /* Private interface revision shared with ptedit.library.
     * Keep this last; bump PARTITION_PRIVATE_ABI whenever the handle,
     * table data or base layouts used by both libraries change. */
    ULONG pb_PrivateABI;
};

#define PARTITION_PRIVATE_ABI 1

#define PBASE(x) ((struct PartitionBase_intern *)x)

#define PTYPE(x) ((struct PartitionType *)x)

/* Last: ptedit.library's libdefs include headers that need the above */
#include LC_LIBDEFS_FILE

#endif /* PARTITION_INTERN_H */

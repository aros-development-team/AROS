#ifndef PARTITION_INTERN_H
#define PARTITION_INTERN_H

/*
    Copyright © 2001-2011, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Internal definitions for partition.library
   Lang: english
*/

#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <libraries/partition.h>

#include <aros/libcall.h>
#include LC_LIBDEFS_FILE

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
};

#define PBASE(x) ((struct PartitionBase_intern *)x)

#define PTYPE(x) ((struct PartitionType *)x)

#endif /* PARTITION_INTERN_H */

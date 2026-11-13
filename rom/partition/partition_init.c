/*
   Copyright (C) 2001-2026, The AROS Development Team. All rights reserved.

   Desc: Partition initialization code
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/alib.h>

#include "partition_intern.h"
#include "partition_support.h"
#include LC_LIBDEFS_FILE

const struct PTFunctionTable * const PartitionSupport[] =
{
    &PartitionGPT, /* The order of these two is important, GPT must be checked before MBR */
    &PartitionMBR,
    &PartitionEBR,
    &PartitionRDB,
    NULL
};

/* Keep RDB last. RDB scans up to 16 blocks and in case of
 * MBR->EBR->RDB having RDB first detects RDB as root and
 * causes MBR and EBR offsets not be taken into account
 */

static int PartitionInit(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->partbase.tables =  (struct PartitionTableInfo **)PartitionSupport;
    LIBBASE->pb_PrivateABI = PARTITION_PRIVATE_ABI;
    NewList(&LIBBASE->bootList);
    InitSemaphore(&LIBBASE->bootSem);

    /*
     * This is intentionally allowed to fail.
     * It will fail if we are in kickstart; partition.library is initialized
     * long before dos.library.
     */
    LIBBASE->pb_DOSBase = TaggedOpenLibrary(TAGGEDOPEN_DOS);

    return TRUE;
}

static int PartitionCleanup(struct PartitionBase_intern *base)
{
    /* If there's something in our boot list, we can't quit without losing it */
    if (!IsListEmpty(&base->bootList))
        return FALSE;

    if (base->pb_DOSBase)
        CloseLibrary(base->pb_DOSBase);

    return TRUE;
}

ADD2INITLIB(PartitionInit, 0);
ADD2EXPUNGELIB(PartitionCleanup, 0);

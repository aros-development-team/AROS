/*
   Copyright (C) 2026, The AROS Development Team. All rights reserved.

   Desc: partitionedit.library initialization code
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "partitionedit_intern.h"
#include LC_LIBDEFS_FILE

static int PartitionEditInit(LIBBASETYPEPTR LIBBASE)
{
    struct Library *PartitionBase = OpenLibrary("partition.library", PARTITION_MIN_VERSION);

    if (!PartitionBase)
        return FALSE;

    /* Handles and table data are shared with partition.library's
     * handlers, so both libraries must agree on their layout. */
    if (PartitionBase->lib_PosSize >= sizeof(struct PartitionBase_intern) &&
        PBASE(PartitionBase)->pb_PrivateABI == PARTITION_PRIVATE_ABI)
    {
        LIBBASE->pe_PartitionBase = PartitionBase;
        return TRUE;
    }

    CloseLibrary(PartitionBase);
    return FALSE;
}

static int PartitionEditExpunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->pe_PartitionBase)
        CloseLibrary(LIBBASE->pe_PartitionBase);

    return TRUE;
}

ADD2INITLIB(PartitionEditInit, 0);
ADD2EXPUNGELIB(PartitionEditExpunge, 0);

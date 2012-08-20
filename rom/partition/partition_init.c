/*
   Copyright © 2001-2011, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Partition initialization code
   Lang: English
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/alib.h>

#include "partition_intern.h"
#include "partition_support.h"
#include LC_LIBDEFS_FILE

static int PartitionInit(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->partbase.tables =  (struct PartitionTableInfo **)PartitionSupport;
    NewList(&LIBBASE->bootList);
    InitSemaphore(&LIBBASE->bootSem);

    /*
     * This is intentionally allowed to fail.
     * It will fail if we are in kickstart; partition.library is initialized
     * long before dos.library.
     */
    LIBBASE->pb_DOSBase = OpenLibrary("dos.library", 36);

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

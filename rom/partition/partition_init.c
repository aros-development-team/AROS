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

    return TRUE;
}

static int PartitionCleanup(struct PartitionBase_intern *base)
{
    if (base->dosBase)
    	CloseLibrary(base->dosBase);

    return TRUE;
}

ADD2INITLIB(PartitionInit, 0);
ADD2EXPUNGELIB(PartitionCleanup, 0);

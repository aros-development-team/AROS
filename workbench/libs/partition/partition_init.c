/*
   Copyright � 2001-2006, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Partition initialization code
   Lang: English
*/

#include <aros/symbolsets.h>

#define DEBUG 1
#include <aros/debug.h>

#include "partition_intern.h"
#include "partition_support.h"
#include LC_LIBDEFS_FILE

static int PartitionInit(LIBBASETYPEPTR LIBBASE)
{
    ((struct PartitionBase *)LIBBASE)->tables = (struct PartitionTableInfo **)PartitionSupport;
    return TRUE;
}

ADD2INITLIB(PartitionInit, 0);

/*
   (C) 2001 AROS - The Amiga Research OS
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

AROS_SET_LIBFUNC(Init, LIBBASETYPE, LIBBASE)
{
    LIBBASE->tables = (struct PartitionTableInfo **)PartitionSupport;
    return TRUE;
}

ADD2INITLIB(Init, 0);


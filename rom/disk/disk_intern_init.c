/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#include <proto/disk.h>
#include <resources/disk.h>

#include "disk_intern.h"

#define DEBUG 0
#include <aros/debug.h>

void readunitid_internal (struct DiscResource *DiskBase, LONG unitNum)
{
	bug("disk.resource not implemented DF%d: ID read attempt\n", unitNum);
}

BOOL disk_internal_init (struct DiscResource *DiskBase)
{
	bug("disk.resource initialization not implemented\n");
	return 0;
}

/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ReadUnitID() function.
    Lang: english
*/

#include <proto/disk.h>
#include <resources/disk.h>

extern void readunitid_internal (struct DiscResource*, LONG unitNum);

AROS_LH1(ULONG, ReadUnitID,
	 AROS_LHA(LONG, unitNum, D0),
	 struct DiscResource *, DiskBase, 6, Disk)
{
    AROS_LIBFUNC_INIT

	readunitid_internal (DiskBase, unitNum);
	return DiskBase->dr_UnitID[unitNum];

    AROS_LIBFUNC_EXIT
}

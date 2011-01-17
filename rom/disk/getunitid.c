/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetUnitID() function.
    Lang: english
*/

#include <proto/disk.h>
#include <resources/disk.h>

AROS_LH1(ULONG, GetUnitID,
	 AROS_LHA(LONG, unitNum, D0),
	 struct DiscResource *, DiskBase, 5, Disk)
{
    AROS_LIBFUNC_INIT

	return DiskBase->dr_UnitID[unitNum];

    AROS_LIBFUNC_EXIT
}

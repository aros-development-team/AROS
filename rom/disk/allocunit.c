/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AllocUnit() function.
    Lang: english
*/

#include <proto/disk.h>
#include <resources/disk.h>

AROS_LH1(BOOL, AllocUnit,
	 AROS_LHA(LONG, unitNum, D0),
	 struct DiscResource *, DiskBase, 1, Disk)
{
    AROS_LIBFUNC_INIT

	if (DiskBase->dr_Flags & (1 << unitNum))
		return 0;
	DiskBase->dr_Flags |= 1 << unitNum;
    return 1;

    AROS_LIBFUNC_EXIT
}

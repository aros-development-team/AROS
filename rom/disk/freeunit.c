/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FreeUnit() function.
    Lang: english
*/

#include <proto/disk.h>
#include <resources/disk.h>

AROS_LH1(void, FreeUnit,
	 AROS_LHA(LONG, unitNum, D0),
	 struct DiscResource *, DiskBase, 2, Disk)
{
    AROS_LIBFUNC_INIT

	DiskBase->dr_Flags &= ~(1 << unitNum);

    AROS_LIBFUNC_EXIT
}

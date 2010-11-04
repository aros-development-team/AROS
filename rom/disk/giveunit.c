/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GiveUnit() function.
    Lang: english
*/

#include <proto/disk.h>
#include <proto/exec.h>
#include <resources/disk.h>

AROS_LH0(void, GiveUnit,
	struct DiscResource *, DiskBase, 4, Disk)
{
    AROS_LIBFUNC_INIT

	struct Task *thistask = FindTask(0);
	struct DiscResourceUnit *dru;

	Disable();
	if (DiskBase->dr_CurrTask == thistask) {
		DiskBase->dr_CurrTask = NULL;	
		DiskBase->dr_Flags &= ~DRF_ACTIVE;
		dru = (struct DiscResourceUnit*)RemHead(&DiskBase->dr_Waiting);
		Enable();
		if (dru)
			ReplyMsg(&dru->dru_Message);
	} else {
		Enable ();
	}

    AROS_LIBFUNC_EXIT
}

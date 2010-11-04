/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetUnit() function.
    Lang: english
*/

#include <proto/disk.h>
#include <proto/exec.h>
#include <resources/disk.h>

AROS_LH1(struct DiscResourceUnit*, GetUnit,
	 AROS_LHA(struct DiscResourceUnit*, unitPointer, A1),
	 struct DiscResource *, DiskBase, 3, Disk)
{
    AROS_LIBFUNC_INIT
    
    struct DiscResourceUnit *old = DiskBase->dr_Current;

	Disable();
	if (DiskBase->dr_Flags & DRF_ACTIVE) {
		AddTail(&DiskBase->dr_Waiting, &unitPointer->dru_Message.mn_Node);
		Enable();
		return NULL;
	}
	DiskBase->dr_Flags |= DRF_ACTIVE;
	DiskBase->dr_Current = unitPointer;
	DiskBase->dr_CurrTask = FindTask(0);
	if (!old)
		old = DiskBase->dr_Current;
	Enable();
	return old;

    AROS_LIBFUNC_EXIT
}

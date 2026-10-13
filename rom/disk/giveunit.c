/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.

    Desc: GiveUnit() function.
*/

#include <proto/disk.h>
#include <proto/exec.h>
#include <resources/disk.h>

/*****************************************************************************

    NAME */
        AROS_LH0(void, GiveUnit,

/*  LOCATION */
        struct DiscResource *, DiskBase, 4, Disk)
/*  FUNCTION
        Release the shared floppy disk hardware owned by the calling
        task. Remove the first waiting descriptor, if any, from the waiting
        list and ReplyMsg() its dru_Message. The notified task must retry
        GetUnit() to obtain ownership.

    INPUTS
        None.

    RESULT
        None. If the calling task is not the recorded owner, no state is
        changed and no waiter is notified.

    NOTES
        Finish disk transfers and arrange the hardware/interrupt state
        for relinquishing ownership before calling. This function only
        updates resource ownership and notifies a waiter; it does not stop
        DMA or disable disk interrupts.

        The current descriptor pointer is retained, and native Amiga disk
        interrupt dispatch still consults it. Releasing ownership does not
        release the descriptor's storage or make it safe to free its
        interrupt code/data. Drive allocations made by AllocUnit() remain
        in force until FreeUnit().

        Call from the task that successfully acquired GetUnit(). Ownership
        and waiting-list updates use Disable()/Enable(); the reply is sent
        after Enable().

    SEE ALSO
        GetUnit(), FreeUnit()

*****************************************************************************/
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

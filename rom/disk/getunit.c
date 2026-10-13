/*
    Copyright (C) 2010, The AROS Development Team. All rights reserved.

    Desc: GetUnit() function.
*/

#include <proto/disk.h>
#include <proto/exec.h>
#include <resources/disk.h>

/*****************************************************************************

    NAME */
        AROS_LH1(struct DiscResourceUnit*, GetUnit,

/*  SYNOPSIS */
         AROS_LHA(struct DiscResourceUnit*, unitPointer, A1),

/*  LOCATION */
         struct DiscResource *, DiskBase, 3, Disk)
/*  FUNCTION
        Try to acquire the shared floppy disk hardware for the calling
        task. If it is available, install the supplied descriptor as the
        current interrupt descriptor. Otherwise append its message to the
        resource's waiting list and return immediately.

    INPUTS
        unitPointer - Non-NULL pointer to a caller-owned DiscResourceUnit.
                      Initialize dru_Message with a valid reply port and
                      initialize the three interrupt descriptors, including
                      their is_Code and is_Data fields. Use a NULL is_Code
                      for an unused interrupt handler. The message must
                      not already be queued or linked on another list.

    RESULT
        On success, a non-NULL pointer to the previous current descriptor.
        If no descriptor was previously installed, unitPointer is returned.
        The returned descriptor is borrowed; ownership is not transferred.

        NULL means the hardware was busy and dru_Message has been queued.
        Wait for and remove the reply to that message before retrying.
        A reply is a notification to retry, not a grant of ownership.

    NOTES
        Call from task context. This is not a recursive acquisition:
        calling again while already owning the hardware queues a request
        rather than granting another ownership level. Do not enqueue the
        same descriptor twice. On successful acquisition, the same task
        must eventually call GiveUnit().

        The descriptor, reply port and interrupt code/data must remain
        valid while queued or installed. GiveUnit() retains the current
        descriptor pointer: do not free it merely because ownership was
        released. In the native Amiga implementation the disk interrupt
        handlers use that pointer even while the resource is inactive.

        Queue and ownership updates are protected with Disable()/Enable().
        The function returns without waiting for ownership; it does not
        perform disk I/O or reserve a drive number.

    BUGS
        The previous descriptor is sampled before the ownership critical
        section. If another task changes the current descriptor before
        acquisition, the returned pointer need not identify the immediately
        preceding owner. Do not use it as a synchronized ownership record.

    SEE ALSO
        GiveUnit(), AllocUnit(), exec.library/WaitPort(),
        exec.library/GetMsg()

*****************************************************************************/
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

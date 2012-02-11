/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#include <devices/timer.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "debug.h"
#include "ntfs_fs.h"
#include "ntfs_protos.h"

LONG InitTimer(void)
{
    LONG err = ERROR_NO_FREE_STORE;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    glob->timerport = CreateMsgPort();
    if (glob->timerport) {
        glob->timereq = (struct timerequest *)CreateIORequest(glob->timerport,
            sizeof(struct timerequest));
        if (glob->timereq) {
            if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)glob->timereq, 0))
                err = ERROR_DEVICE_NOT_MOUNTED;
            else {
                glob->timer_active = 0;
                glob->restart_timer = 1;
                D(bug("[NTFS] %s: Timer ready\n", __PRETTY_FUNCTION__));
                return 0;
            }
            DeleteIORequest((struct IORequest *)glob->timereq);
        }
        DeleteMsgPort(glob->timerport);
    }
    return err;
}

void CleanupTimer(void)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    D(bug("[NTFS] %s: Cleaning up timer\n", __PRETTY_FUNCTION__));
    if (glob->timer_active) {
        D(bug("[NTFS] %s: Terminating active request\n", __PRETTY_FUNCTION__));
        AbortIO((struct IORequest *)glob->timereq);
        WaitIO((struct IORequest *)glob->timereq);
    }
    CloseDevice((struct IORequest *)glob->timereq);
    DeleteIORequest((struct IORequest *)glob->timereq);
    DeleteMsgPort(glob->timerport);
}

void RestartTimer(void)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (glob->timer_active) {
        D(bug("[NTFS] %s: Queuing timer restart\n", __PRETTY_FUNCTION__));
        glob->restart_timer = 1;
    } else {
        D(bug("[NTFS] %s: Immediate timer restart\n", __PRETTY_FUNCTION__));
        glob->timereq->tr_node.io_Command = TR_ADDREQUEST;
        glob->timereq->tr_time.tv_secs = 1;
        glob->timereq->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)glob->timereq);
        glob->timer_active = 1;
    }
}

void HandleTimer(void)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    WaitIO((struct IORequest *)glob->timereq);
    glob->timer_active = 0;
    if (glob->restart_timer) {
        D(bug("[NTFS] %s: Timer restart queued\n", __PRETTY_FUNCTION__));
        glob->restart_timer = 0;
        RestartTimer();
    } else {
        D(bug("[NTFS] %s: Updating disk\n", __PRETTY_FUNCTION__));
        UpdateDisk();
    }
}


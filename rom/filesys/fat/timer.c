/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright   2008-2010 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#define DEBUG 0

#include <devices/timer.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "debug.h"
#include "fat_fs.h"
#include "fat_protos.h"

LONG InitTimer(void)
{
    LONG err = ERROR_NO_FREE_STORE;

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
                D(bug("[fat] Timer ready\n"));
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
    D(bug("[fat] Cleaning up timer\n"));
    if (glob->timer_active) {
        D(bug("[fat] Terminating active request\n"));
        AbortIO((struct IORequest *)glob->timereq);
        WaitIO((struct IORequest *)glob->timereq);
    }
    CloseDevice((struct IORequest *)glob->timereq);
    DeleteIORequest((struct IORequest *)glob->timereq);
    DeleteMsgPort(glob->timerport);
}

void RestartTimer(void)
{
    if (glob->timer_active) {
        D(bug("Queuing timer restart\n"));
        glob->restart_timer = 1;
    } else {
        D(bug("Immediate timer restart\n"));
        glob->timereq->tr_node.io_Command = TR_ADDREQUEST;
        glob->timereq->tr_time.tv_secs = 1;
        glob->timereq->tr_time.tv_micro = 0;
        SendIO((struct IORequest *)glob->timereq);
        glob->timer_active = 1;
    }
}

void HandleTimer(void)
{
    WaitIO((struct IORequest *)glob->timereq);
    glob->timer_active = 0;
    if (glob->restart_timer) {
        D(bug("Timer restart queued\n"));
        glob->restart_timer = 0;
        RestartTimer();
    } else {
        D(bug("Updating disk\n"));
        UpdateDisk();
    }
}


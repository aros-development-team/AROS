/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Input device
    Lang: english
*/

#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/newstyle.h>
#include <proto/exec.h>
#include <proto/input.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/initializers.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

#include "input_intern.h"

#define DEBUG 0
#include <aros/debug.h>

#define NEWSTYLE_DEVICE 1

#if NEWSTYLE_DEVICE

#include <devices/newstyle.h>

static const UWORD SupportedCommands[] =
{
    IND_ADDHANDLER,
    IND_REMHANDLER,
    IND_WRITEEVENT,
    IND_ADDEVENT,
    IND_SETTHRESH,
    IND_SETPERIOD,
    NSCMD_DEVICEQUERY,
    0
};

#endif


static int GM_UNIQUENAME(Init) (LIBBASETYPEPTR InputDevice)
{
    NEWLIST(&(InputDevice->HandlerList));

    /*
       These defaults are in terms of 50 Hz ticks. The real VBlank frequency
       does not affect them.
     */
    InputDevice->KeyRepeatThreshold.tv_secs =
        DEFAULT_KEY_REPEAT_THRESHOLD / 50;
    InputDevice->KeyRepeatThreshold.tv_micro =
        (DEFAULT_KEY_REPEAT_THRESHOLD % 50) * 1000000 / 50;

    InputDevice->KeyRepeatInterval.tv_secs =
        DEFAULT_KEY_REPEAT_INTERVAL / 50;
    InputDevice->KeyRepeatInterval.tv_micro =
        (DEFAULT_KEY_REPEAT_INTERVAL % 50) * 1000000 / 50;

    D(bug("[InputDev] Starting up task, inputbase 0x%P\n", InputDevice));

    InputDevice->InputTask = NewCreateTask(TASKTAG_NAME, "input.device",
        TASKTAG_PRI, IDTASK_PRIORITY,
        TASKTAG_STACKSIZE, IDTASK_STACKSIZE,
        TASKTAG_TASKMSGPORT, &InputDevice->CommandPort,
        TASKTAG_PC, ProcessEvents, TASKTAG_ARG1, InputDevice, TAG_DONE);

    if (InputDevice->InputTask)
    {
        D(bug("[InputDev] Done\n"));
        return TRUE;
    }

    return FALSE;
}


static int GM_UNIQUENAME(Open)
    (LIBBASETYPEPTR InputDevice,
    struct IORequest *ioreq, ULONG unitnum, ULONG flags)
{
    D(bug("id: open()\n"));
#ifndef __mc68000
    /* Too many AOS programs do not initialize mn_Length. AOS input.device
       does not care. */
    if (ioreq->io_Message.mn_Length < sizeof(struct IOStdReq))
    {
        bug("[InputDev] Open: IORequest structure passed to OpenDevice is"
            " too small\n");
        ioreq->io_Error = IOERR_OPENFAIL;
        return FALSE;
    }
#endif
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)

#define ioStd(x)  ((struct IOStdReq *)x)


AROS_LH1(void, beginio,
    AROS_LHA(struct IOStdReq *, ioreq, A1),
    struct inputbase *, InputDevice, 5, Input)
{
    AROS_LIBFUNC_INIT

    LONG error = 0;
    BOOL done_quick = TRUE;

    D(bug("id: beginio(ioreq=%p)\n", ioreq));

    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;

    switch (ioreq->io_Command)
    {
#if NEWSTYLE_DEVICE
    case NSCMD_DEVICEQUERY:
        if (ioStd(ioreq)->io_Length < ((LONG) OFFSET(NSDeviceQueryResult,
                    SupportedCommands)) + sizeof(UWORD *))
        {
            ioreq->io_Error = IOERR_BADLENGTH;
        }
        else
        {
            struct NSDeviceQueryResult *d;

            d = (struct NSDeviceQueryResult *)ioStd(ioreq)->io_Data;

            d->DevQueryFormat = 0;
            d->SizeAvailable = sizeof(struct NSDeviceQueryResult);
            d->DeviceType = NSDEVTYPE_INPUT;
            d->DeviceSubType = 0;
            d->SupportedCommands = (UWORD *) SupportedCommands;

            ioStd(ioreq)->io_Actual = sizeof(struct NSDeviceQueryResult);
        }
        break;
#endif

    case IND_ADDHANDLER:
    case IND_REMHANDLER:
    case IND_WRITEEVENT:
    case IND_ADDEVENT:
    case IND_SETTHRESH:
    case IND_SETPERIOD:
        done_quick = FALSE;
        break;

    default:
        error = IOERR_NOCMD;
        break;
    }

    if (!done_quick)
    {
        /* Mark IO request to be done non-quick */
        ioreq->io_Flags &= ~IOF_QUICK;
        /* Send to input device task */
        PutMsg(InputDevice->CommandPort, &ioreq->io_Message);
    }
    else
    {

        /* If the quick bit is not set but the IO request was done quick,
         * reply the message to tell we're throgh
         */
        ioreq->io_Error = error;
        if (!(ioreq->io_Flags & IOF_QUICK))
            ReplyMsg(&ioreq->io_Message);
    }

    D(bug("id: Return from BeginIO()\n"));

    AROS_LIBFUNC_EXIT
}


AROS_LH1(LONG, abortio,
    AROS_LHA(struct IORequest *, ioreq, A1),
    struct inputbase *, InputDevice, 6, Input)
{
    AROS_LIBFUNC_INIT

    /* Everything already done. */
    return 0;

    AROS_LIBFUNC_EXIT
}

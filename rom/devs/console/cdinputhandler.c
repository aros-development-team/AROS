/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: console.device function CDInputHandler()
    Lang: english
*/

#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/console.h>
#include <proto/intuition.h>
#include <intuition/intuitionbase.h>

#include <aros/asmcall.h>

#include <devices/inputevent.h>

#include "console_gcc.h"

#define DEBUG 0
#include <aros/debug.h>

/* protos */
static Object *obtainconunit(struct ConsoleBase *ConsoleDevice);
static VOID releaseconunit(Object *o, struct ConsoleBase *ConsoleDevice);

/*************************************************************************

    NAME */
        AROS_LH2I(struct InputEvent *, CDInputHandler,

/*  SYNOPSIS */
        AROS_LHA(struct InputEvent *, events, A0),
        AROS_LHA(APTR, _cdihdata, A1),

/*  LOCATION */
        struct Library *, ConsoleDevice, 7, Console)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct cdihData *cdihdata = _cdihdata;

#undef ConsoleDevice
#define ConsoleDevice (cdihdata->consoleDevice)

    struct InputEvent *ie;
    BOOL send_message = FALSE;

    struct cdihMessage *message = cdihdata->cdihMsg;
    D(bug("CDInputHandler(events=%p, cdihdata=%p)\n", events, cdihdata));

    for (ie = events; ie; ie = ie->ie_NextEvent)
    {
        /* A rawkey event ? */
        if ((ie->ie_Class == IECLASS_RAWKEY
                && !(ie->ie_Code & IECODE_UP_PREFIX))
            || (ie->ie_Class == IECLASS_SIZEWINDOW)
            || (ie->ie_Class == IECLASS_CLOSEWINDOW)
            || (ie->ie_Class == IECLASS_REFRESHWINDOW)
            || (ie->ie_Class == IECLASS_GADGETDOWN)
            || (ie->ie_Class == IECLASS_GADGETUP)
            || (ie->ie_Class == IECLASS_RAWMOUSE)
            || (ie->ie_Class == IECLASS_TIMER))
        {
            /* What console do we send it to ? */
            Object *unit;

            D(bug("Got some event\n"));
            /* find and prevent deletion of unit */
            unit = obtainconunit(ConsoleDevice);
            if (unit)
            {
                message->unit = unit;
                message->ie = *ie;
                send_message = TRUE;

                D(bug("Event should be passed to unit %p\n", unit));

                /* deletion of unit is now allowed */
                releaseconunit(unit, ConsoleDevice);

            } /* if (RAWKEY event was meant for a console window) */
        }
        else
        {
            D(bug("Ignoring event of ie_Class %d\n", ie->ie_Class));
        }

        if (send_message)
        {
            /* This function might be called by any task, not only
               by the input.device, so it is important that
               we initialize the replyport's task each time.
             */
            struct MsgPort *replyport;

            replyport = message->msg.mn_ReplyPort;

            replyport->mp_SigTask = FindTask(NULL);
            PutMsg(cdihdata->inputPort, (struct Message *)message);

            /* Wait for reply */
            WaitPort(replyport);

            /* Remove it from the replyport's msgqueue */
            GetMsg(replyport);

            send_message = FALSE;
        }
    } /* for (each event in the chain) */

    ReturnPtr("CDInputHandler", struct InputEvent *, events);

    AROS_LIBFUNC_EXIT
} /* CDInputHandler */



#undef ConsoleDevice
#undef IntuitionBase

/***********************
**  Support funtions  **
***********************/

/* Obtains a conunit object, and locks it, so that it's
   not deleted while we work on it
*/

static Object *obtainconunit(struct ConsoleBase *ConsoleDevice)
{
    struct IntuitionBase *IntuitionBase =
        (APTR) ConsoleDevice->cb_IntuitionBase;
    struct Window *activewin;
    Object *o, *ostate;
    ULONG lock;
    struct Node *node;

    D(bug("obtainconunit()\n"));

    ForeachNode(&ConsoleDevice->unitList, node)
    {
        D(bug("Node: %p\n", node));
    }

    /* Lock the console list */
    ObtainSemaphoreShared(&ConsoleDevice->unitListLock);

    /* What is the currently active window ? */
    D(bug("Obtaining IBase\n"));
    lock = LockIBase(0UL);

    activewin = IntuitionBase->ActiveWindow;

    UnlockIBase(lock);
    D(bug("Released IBase, active win=%p\n", activewin));

    /* Try to find the correct unit object for taht window */
    ostate = (Object *) ConsoleDevice->unitList.mlh_Head;

    D(bug("Searching for con unit\n"));
    while ((o = NextObject(&ostate)))
    {
        D(bug("Trying unit %p, win=%p\n", o, CU(o)->cu_Window));
        /* Is this console the currently active window ? */
        if (CU(o)->cu_Window == activewin)
        {
            D(bug("Unit found: %p\n", o));
            /* Delay deltion of this console object */
            ICU(o)->conFlags |= CF_DELAYEDDISPOSE;
            break;
        }
    }

    /* Unlock the console list */
    ReleaseSemaphore(&ConsoleDevice->unitListLock);

    ReturnPtr("obtainconunit", Object *, o);
}

static VOID releaseconunit(Object *o, struct ConsoleBase *ConsoleDevice)
{
    struct IntuitionBase *IntuitionBase =
        (APTR) ConsoleDevice->cb_IntuitionBase;

    /* Lock all units */
    ObtainSemaphore(&ConsoleDevice->unitListLock);

    /* Needn't prevent the unit from being disposed anymore */
    ICU(o)->conFlags &= ~CF_DELAYEDDISPOSE;

    /* If unit is scheduled for deletion, then delete it */
    if (ICU(o)->conFlags & CF_DISPOSE)
    {
        ULONG mID = OM_REMOVE;

        /* Remove from list */
        DoMethodA(o, (Msg) &mID);

        /* Delete it */
        DisposeObject(o);
    }

    ReleaseSemaphore(&ConsoleDevice->unitListLock);
}

/****************
** initCDIH()  **
****************/
/* This function should be executed on the console.device task's context only,
   so that the inputport is set correctly
*/

struct Interrupt *initCDIH(struct ConsoleBase *ConsoleDevice)
{
    struct Interrupt *cdihandler;
    struct cdihData *cdihdata;

    D(bug("initCDIH(ConsoleDevice=%p)\n", ConsoleDevice));

    cdihandler =
        AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC | MEMF_CLEAR);
    if (cdihandler)
    {
        cdihdata =
            AllocMem(sizeof(struct cdihData), MEMF_PUBLIC | MEMF_CLEAR);
        if (cdihdata)
        {
            cdihdata->inputPort = CreateMsgPort();
            if (cdihdata->inputPort)
            {
                struct cdihMessage *msg;

                msg =
                    AllocMem(sizeof(struct cdihMessage),
                    MEMF_PUBLIC | MEMF_CLEAR);
                if (msg)
                {
                    struct MsgPort *port;
                    port =
                        AllocMem(sizeof(struct MsgPort),
                        MEMF_PUBLIC | MEMF_CLEAR);
                    if (port)
                    {
                        /* Initialize port */
                        port->mp_Flags = PA_SIGNAL;
                        port->mp_SigBit = SIGB_INTUITION;

                        /* The task of the replyport must be initialized each
                           time used, because the CDInputHandler might be
                           called by an app
                         */

                        NEWLIST(&(port->mp_MsgList));

                        cdihdata->cdihReplyPort = port;

                        /* Initialize Message struct */
                        cdihdata->cdihMsg = msg;
                        msg->msg.mn_ReplyPort = cdihdata->cdihReplyPort;
                        msg->msg.mn_Length = sizeof(struct cdihMessage);

                        /* Initialize Interrupt struct */
                        cdihandler->is_Code =
                            (VOID_FUNC) AROS_SLIB_ENTRY(CDInputHandler,
                            Console, 7);
                        cdihandler->is_Data = cdihdata;
                        cdihandler->is_Node.ln_Pri = 50;
                        cdihandler->is_Node.ln_Name =
                            "console.device InputHandler";

                        cdihdata->consoleDevice = ConsoleDevice;

                        ReturnPtr("initCDIH", struct Interrupt *,
                            cdihandler);
                    }
                    FreeMem(cdihdata->cdihMsg, sizeof(struct cdihMessage));
                }
                DeleteMsgPort(cdihdata->inputPort);
            }
            FreeMem(cdihdata, sizeof(struct cdihData));
        }
        FreeMem(cdihandler, sizeof(struct Interrupt));
    }
    ReturnPtr("initCDIH", struct Interrupt *, NULL);
}

/****************
** CleanupIIH  **
****************/

VOID cleanupCDIH(struct Interrupt *cdihandler,
    struct ConsoleBase *ConsoleDevice)
{
    struct cdihData *cdihdata;

    cdihdata = (struct cdihData *)cdihandler->is_Data;

    FreeMem(cdihdata->cdihReplyPort, sizeof(struct MsgPort));

    FreeMem(cdihdata->cdihMsg, sizeof(struct cdihMessage));

    DeleteMsgPort(cdihdata->inputPort);

    FreeMem(cdihandler->is_Data, sizeof(struct cdihData));
    FreeMem(cdihandler, sizeof(struct Interrupt));

    return;
}

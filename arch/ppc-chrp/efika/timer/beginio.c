/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BeginIO - Start up a timer.device request.
    Lang: english
*/

#include "lowlevel.h"
#include <devices/newstyle.h>
#include <exec/errors.h>
#include <exec/initializers.h>
#include <proto/exec.h>
#include <asm/io.h>
#include <asm/mpc5200b.h>

/* See rom/timer/beginio.c for documentation */

/****************************************************************************************/

#define NEWSTYLE_DEVICE 1

#define ioStd(x)        ((struct IOStdReq *)x)

/****************************************************************************************/

#if NEWSTYLE_DEVICE

static const UWORD SupportedCommands[] =
{
    TR_GETSYSTIME,
    TR_SETSYSTIME,
    TR_ADDREQUEST,
    NSCMD_DEVICEQUERY,
    0
};

#endif

/****************************************************************************************/

BOOL timer_addToWaitList(struct TimerBase *, struct MinList *, struct timerequest *);

#include <devices/timer.h>
#include <proto/timer.h>

AROS_LH1(void, BeginIO,
        AROS_LHA(struct timerequest *, timereq, A1),
        struct TimerBase *, TimerBase, 5, Timer)
{
    AROS_LIBFUNC_INIT

    ULONG unitNum;
    BOOL replyit = FALSE;

    uint32_t initial_time = mftbl();

    Disable();
    EClockUpdate(TimerBase);
    Enable();

    timereq->tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    timereq->tr_node.io_Error = 0;

    unitNum = (ULONG)timereq->tr_node.io_Unit;

    switch(timereq->tr_node.io_Command)
    {
#if NEWSTYLE_DEVICE
        case NSCMD_DEVICEQUERY:
#warning In timer.device this is maybe a bit problematic, as the timerequest structure does not have io_Data and io_Length members

            if (timereq->tr_node.io_Message.mn_Length < sizeof(struct IOStdReq))
            {
                timereq->tr_node.io_Error = IOERR_BADLENGTH;
            }
            else if(ioStd(timereq)->io_Length < ((LONG)OFFSET(NSDeviceQueryResult, SupportedCommands)) + sizeof(UWORD *))
            {
                timereq->tr_node.io_Error = IOERR_BADLENGTH;
            }
            else
            {
                struct NSDeviceQueryResult *d;

                d = (struct NSDeviceQueryResult *)ioStd(timereq)->io_Data;

                d->DevQueryFormat        = 0;
                d->SizeAvailable         = sizeof(struct NSDeviceQueryResult);
                d->DeviceType            = NSDEVTYPE_TIMER;
                d->DeviceSubType         = 0;
                d->SupportedCommands     = (UWORD *)SupportedCommands;

            ioStd(timereq)->io_Actual = sizeof(struct NSDeviceQueryResult);
            }
            break;
#endif

        case TR_GETSYSTIME:
	    Disable();
            EClockUpdate(TimerBase);
	    Enable();
            GetSysTime(&timereq->tr_time);

            if(!(timereq->tr_node.io_Flags & IOF_QUICK))
            {
                ReplyMsg((struct Message *)timereq);
            }
            replyit = FALSE; /* Because replyit will clear the timeval */
            break;

        case TR_SETSYSTIME:
            Disable();
            TimerBase->tb_CurrentTime.tv_secs = timereq->tr_time.tv_secs;
            TimerBase->tb_CurrentTime.tv_micro = timereq->tr_time.tv_micro;
            EClockSet(TimerBase);
            Enable();
            replyit = TRUE;
            break;

        case TR_ADDREQUEST:
            switch(unitNum)
            {
                case UNIT_WAITUNTIL:
                    /* Firstly, check to see if request is for past */
                    Disable();
                    if(CmpTime(&TimerBase->tb_CurrentTime, &timereq->tr_time) <= 0)
                    {
                        Enable();
                        timereq->tr_time.tv_secs = timereq->tr_time.tv_micro = 0;
                        timereq->tr_node.io_Error = 0;
                        replyit = TRUE;
                    }
                    else
                    {
                        /* Ok, we add this to the list */
                        if (timer_addToWaitList(TimerBase, &TimerBase->tb_Lists[TL_WAITVBL], timereq))
                        {
                            TimerSetup(TimerBase, initial_time);
                        }
                        Enable();
                        replyit = FALSE;
                        timereq->tr_node.io_Flags &= ~IOF_QUICK;
                    }
                    break;

                case UNIT_MICROHZ:
                case UNIT_VBLANK:
                    Disable();
                    AddTime(&timereq->tr_time, &TimerBase->tb_Elapsed);
                    /* Slot it into the list */
                    if (timer_addToWaitList(TimerBase, &TimerBase->tb_Lists[TL_VBLANK], timereq))
                    {
                        TimerSetup(TimerBase, initial_time);
                    }
                    Enable();
                    timereq->tr_node.io_Flags &= ~IOF_QUICK;
                    replyit = FALSE;
                    break;

                case UNIT_ECLOCK:
                case UNIT_WAITECLOCK:
                default:
                    replyit = FALSE;
                    timereq->tr_node.io_Error = IOERR_NOCMD;
                    break;
            } /* switch(unitNum) */
            break;

        case CMD_CLEAR:
        case CMD_FLUSH:
        case CMD_INVALID:
        case CMD_READ:
        case CMD_RESET:
        case CMD_START:
        case CMD_STOP:
        case CMD_UPDATE:
        case CMD_WRITE:
        default:
            replyit = TRUE;
            timereq->tr_node.io_Error = IOERR_NOCMD;
            break;
    } /* switch(command) */

    if(replyit)
    {
        timereq->tr_time.tv_secs = 0;
        timereq->tr_time.tv_micro = 0;
        if(!(timereq->tr_node.io_Flags & IOF_QUICK))
        {
            ReplyMsg((struct Message *)timereq);
        }
    }

    AROS_LIBFUNC_EXIT
} /* BeginIO */

BOOL
timer_addToWaitList(struct TimerBase *TimerBase,
                    struct MinList *list,
                    struct timerequest *iotr)
{
    /* We are disabled, so we should take as little time as possible. */
    struct timerequest *tr;
    BOOL added = FALSE;

    ForeachNode(list, tr)
    {
        /* If the time in the new request is less than the next request */
        if(CmpTime(&tr->tr_time, &iotr->tr_time) < 0)
        {
            /* Add the node before the next request */
            Insert(
                        (struct List *)list,
                        (struct Node *)iotr,
                        tr->tr_node.io_Message.mn_Node.ln_Pred
            );
            added = TRUE;
            break;
        }
    }

    /*
        This will catch the case of either an empty list, or request is
        for after all other requests
    */

    if(!added)
        AddTail((struct List *)list, (struct Node *)iotr);

    /* Return TRUE if it ended up on head of list */

    return ((struct timerequest *)list->mlh_Head == iotr) ? TRUE : FALSE;

}

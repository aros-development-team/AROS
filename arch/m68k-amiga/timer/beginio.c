/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BeginIO - Start up a timer.device request.
    Lang: english
*/

#include <exec/errors.h>
#include <exec/initializers.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <devices/newstyle.h>

#include <timer_intern.h>

#define DEBUG 0
#include <aros/debug.h>

static void timer_addToWaitList(struct TimerBase *TimerBase, UWORD unit, struct timerequest *tr);

#define NEWSTYLE_DEVICE 1

#define ioStd(x)  	((struct IOStdReq *)x)

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


AROS_LH1(void, BeginIO,
	AROS_LHA(struct timerequest *, timereq, A1),
	struct TimerBase *, TimerBase, 5, Timer)

{
    AROS_LIBFUNC_INIT
    
    ULONG unitNum;
    BOOL replyit = FALSE;
 
    timereq->tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    timereq->tr_node.io_Error = 0;
    
    unitNum = (ULONG)timereq->tr_node.io_Unit;
    
    D(bug("timer: %d %d %x %d/%d task: '%s'\n", unitNum, timereq->tr_node.io_Command,
    	timereq, timereq->tr_time.tv_secs, timereq->tr_time.tv_micro, FindTask(0)->tc_Node.ln_Name));
    
    switch(timereq->tr_node.io_Command)
    {
#if NEWSTYLE_DEVICE
        case NSCMD_DEVICEQUERY:
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
		
    		d->DevQueryFormat 	 = 0;
    		d->SizeAvailable 	 = sizeof(struct NSDeviceQueryResult);
    		d->DeviceType 	 	 = NSDEVTYPE_TIMER;
    		d->DeviceSubType 	 = 0;
    		d->SupportedCommands 	 = (UWORD *)SupportedCommands;
    
            ioStd(timereq)->io_Actual = sizeof(struct NSDeviceQueryResult);
   	    }
	    break;
#endif

        case TR_GETSYSTIME:
            GetSysTime(&timereq->tr_time);
            if(!(timereq->tr_node.io_Flags & IOF_QUICK)) {
                ReplyMsg((struct Message *)timereq);
            }
            replyit = FALSE; /* Because replyit will clear the timeval */
            break;
        
        case TR_SETSYSTIME:
            Disable();
            TimerBase->tb_CurrentTime.tv_secs = timereq->tr_time.tv_secs;
            TimerBase->tb_CurrentTime.tv_micro = timereq->tr_time.tv_micro;
            Enable();
            replyit = TRUE;
            break;
        
        case TR_ADDREQUEST:             
            switch(unitNum)
            {
                case UNIT_WAITUNTIL:
                {
                    convertunits(TimerBase, &timereq->tr_time, UNIT_VBLANK);
                    /* Firstly, check to see if request is for past */
                    Disable();
                    if(!cmp64(&TimerBase->tb_vb_count, &timereq->tr_time)) {
                        Enable();
                        timereq->tr_time.tv_secs = timereq->tr_time.tv_micro = 0;
                        timereq->tr_node.io_Error = 0;
                        replyit = TRUE;
                    } else {
                        timer_addToWaitList(TimerBase, UNIT_VBLANK, timereq);
                        Enable();
                        replyit = FALSE;
                        timereq->tr_node.io_Flags &= ~IOF_QUICK;
                    }
                    break;
        	}
                case UNIT_MICROHZ:
                    convertunits(TimerBase, &timereq->tr_time, UNIT_MICROHZ);
                    Disable();
                    addmicro(TimerBase, &timereq->tr_time);
                    timer_addToWaitList(TimerBase, UNIT_MICROHZ, timereq);
                    Enable();
                    timereq->tr_node.io_Flags &= ~IOF_QUICK;
                    replyit = FALSE;
                    break;
                case UNIT_VBLANK:
                    convertunits(TimerBase, &timereq->tr_time, UNIT_VBLANK);
                    add64(&timereq->tr_time, &TimerBase->tb_vb_count);
	            Disable();
                    timer_addToWaitList(TimerBase, UNIT_VBLANK, timereq);
                    Enable();
                    timereq->tr_node.io_Flags &= ~IOF_QUICK;
                    replyit = FALSE;
                    break;
                case UNIT_ECLOCK:
	            Disable();
                    addmicro(TimerBase, &timereq->tr_time);
                    timer_addToWaitList(TimerBase, UNIT_MICROHZ, timereq);
                    Enable();
                    timereq->tr_node.io_Flags &= ~IOF_QUICK;
                    replyit = FALSE;
		    break;
               	case UNIT_WAITECLOCK:
	            Disable();
                    timer_addToWaitList(TimerBase, UNIT_MICROHZ, timereq);
                    Enable();
                    timereq->tr_node.io_Flags &= ~IOF_QUICK;
                    replyit = FALSE;
                    break;
                default:
                    replyit = TRUE;
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


static void timer_addToWaitList(struct TimerBase *TimerBase, UWORD unit, struct timerequest *iotr)
{
    /* We are disabled, so we should take as little time as possible. */
    struct timerequest *tr;
    BOOL added = FALSE, first = TRUE;
    struct MinList *list = &TimerBase->tb_Lists[unit];

    if (unit == UNIT_VBLANK) {
	// always wait at least 1 full vblank
	if (equ64(&iotr->tr_time, &TimerBase->tb_vb_count))
	    inc64(&iotr->tr_time);
	inc64(&iotr->tr_time);
    }
 
    ForeachNode(list, tr) {
    	/* If the time in the new request is less than the next request */
    	if(CmpTime(&tr->tr_time, &iotr->tr_time) < 0) {
    	    /* Add the node before the next request */
    	    Insert((struct List *)list, (struct Node *)iotr, tr->tr_node.io_Message.mn_Node.ln_Pred);
    	    added = TRUE;
    	    break;
    	}
    	first = FALSE;
    }

    /*
	This will catch the case of either an empty list, or request is
	for after all other requests
    */

    if(!added)
    	AddTail((struct List *)list, (struct Node *)iotr);

    /* recalculate timers, list was empty or was added to head of list */
    if (!added || first)
	CheckTimer(TimerBase, unit);   

    D(bug("added %x: %d/%d->%d/%d\n", iotr,
	(unit == UNIT_VBLANK ? TimerBase->tb_vb_count.tv_secs : TimerBase->tb_micro_count.tv_secs),
	(unit == UNIT_VBLANK ? TimerBase->tb_vb_count.tv_usec : TimerBase->tb_micro_count.tv_usec),
	iotr->tr_time.tv_secs, iotr->tr_time.tv_micro));

}

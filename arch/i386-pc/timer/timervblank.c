/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: timervblank.c 22960 2005-02-23 14:25:54Z jmj $

    Desc: VBlank server for the timer.device/timer.hidd
    Lang: english
*/
#include <exec/types.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <aros/debug.h>

#include "ticks.h"
#undef SysBase

/*
    This is a slightly faster version of AddTime() that we use here. It
    also saves the function call overhead...
*/
#define FastAddTime(d, s)\
    (d)->tv_micro += (s)->tv_micro;\
    (d)->tv_secs += (s)->tv_secs;\
    if((d)->tv_micro > 999999) {\
	(d)->tv_secs++;\
	(d)->tv_micro -= 1000000;\
    }

BOOL timer_addToWaitList(struct TimerBase *, struct MinList *, struct timerequest *);

AROS_UFH4(ULONG, VBlankInt,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
    )
{
    AROS_USERFUNC_INIT

    struct timerequest *tr, *next;

    EClockUpdate(TimerBase);

    /*
    	Go through the "wait for x seconds" list and return requests
    	that have completed. A completed request is one whose time
    	is less than that of the elapsed time.
    */
    ForeachNodeSafe(&TimerBase->tb_Lists[TL_VBLANK], tr, next)
    {
        if(CmpTime(&TimerBase->tb_Elapsed, &tr->tr_time) <= 0)
        {
	    if (tr == &TimerBase->tb_vblank_timerequest)
	    {
	    	struct IntVector *iv = &SysBase->IntVects[INTB_VERTB];

	    	/* VBlank Emu */
		
		if (iv->iv_Code)
		{
		    AROS_UFC5(void, iv->iv_Code,
			AROS_UFCA(ULONG, 0, D1),
			AROS_UFCA(ULONG, 0, A0),
			AROS_UFCA(APTR, iv->iv_Data, A1),
			AROS_UFCA(APTR, iv->iv_Code, A5),
			AROS_UFCA(struct ExecBase *, SysBase, A6)
		    );
		}
		
		/* Automatically requeue/reactivate request */
		
		Remove((struct Node *)tr);
		tr->tr_node.io_Command = TR_ADDREQUEST;
		tr->tr_time.tv_secs = 0;
		tr->tr_time.tv_micro = 1000000 / SysBase->VBlankFrequency;
                AddTime(&tr->tr_time, &TimerBase->tb_Elapsed);
		
		timer_addToWaitList(TimerBase, &TimerBase->tb_Lists[TL_VBLANK], tr);	
		
	    }
	    else
	    {
            	/* This request has finished */
            	Remove((struct Node *)tr);
            	tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
            	tr->tr_node.io_Error = 0;
            	ReplyMsg((struct Message *)tr);
	    }
        }
        else
        {
            /*
            	The first request hasn't finished, as all requests are in
            	order, we don't bother searching through the remaining
            */
            break;
        }
    }

    /*
	The other this is the "wait until a specified time". Here a request
	is complete if the time we are waiting for is before the current time.
    */
    ForeachNodeSafe(&TimerBase->tb_Lists[TL_WAITVBL], tr, next)
    {
        if(CmpTime(&TimerBase->tb_CurrentTime, &tr->tr_time) <= 0)
    	{
    	    /* This request has finished */
    	    Remove((struct Node *)tr);
    	    tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
    	    tr->tr_node.io_Error = 0;
    	    ReplyMsg((struct Message *)tr);
    	}
    	else
    	{
    	    /*
    		The first request hasn't finished, as all requests are in
    		order, we don't bother searching through the remaining
    	    */
    	    break;
    	}
    }

    Timer0Setup(TimerBase);
    
    return 0;
    AROS_USERFUNC_EXIT
}
	

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VBlank server for the timer.device/timer.hidd
    Lang: english
*/
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <aros/debug.h>

#include "timer_intern.h"
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

AROS_UFH4(ULONG, VBlankInt,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
    )
{
    AROS_USERFUNC_INIT

    struct timerequest *tr, *next;

    /*
	Firstly increment the current time. No need to Disable() here as
	there are no other interrupts that are allowed to interrupt us
	that can do anything with this.
    */
    FastAddTime(&TimerBase->tb_CurrentTime, &TimerBase->tb_VBlankTime);
    FastAddTime(&TimerBase->tb_Elapsed, &TimerBase->tb_VBlankTime);

    TimerBase->tb_ticks_total++;
    
    /*
	Go through the "wait for x seconds" list and return requests
	that have completed. A completed request is one whose time
	is less than that of the elapsed time.
    */
    tr = (struct timerequest *)TimerBase->tb_Lists[TL_VBLANK].mlh_Head;

    while(tr && ((struct Node *)tr)->ln_Succ != NULL)
    {
	if(     (tr->tr_time.tv_secs < TimerBase->tb_Elapsed.tv_secs)
	    ||	(    (tr->tr_time.tv_secs <= TimerBase->tb_Elapsed.tv_secs)
	         &&  (tr->tr_time.tv_micro < TimerBase->tb_Elapsed.tv_micro))
	  )
	{
	    /* This request has finished */
	    next = (struct timerequest *)tr->tr_node.io_Message.mn_Node.ln_Succ;
	    Remove((struct Node *)tr);
	    tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
	    tr->tr_node.io_Error = 0;
	    ReplyMsg((struct Message *)tr);

	    tr = next;
	}
	else
	{
	    /*
		The first request hasn't finished, as all requests are in
		order, we don't bother searching through the remaining
	    */
	    tr = NULL;
	}
    }

    /*
	The other this is the "wait until a specified time". Here a request
	is complete if the time we are waiting for is before the current time.
    */
    tr = (struct timerequest *)TimerBase->tb_Lists[TL_WAITVBL].mlh_Head;

    while(tr && ((struct Node *)tr)->ln_Succ != NULL)
    {
	if(	(tr->tr_time.tv_secs <= TimerBase->tb_CurrentTime.tv_secs)
	    &&  (tr->tr_time.tv_micro < TimerBase->tb_CurrentTime.tv_micro)
	  )
	{
	    /* This request has finished */
	    next = (struct timerequest *)tr->tr_node.io_Message.mn_Node.ln_Succ;
	    Remove((struct Node *)tr);
	    tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
	    tr->tr_node.io_Error = 0;
	    ReplyMsg((struct Message *)tr);

	    tr = next;
	}
	else
	{
	    /*
		The first request hasn't finished, as all requests are in
		order, we don't bother searching through the remaining
	    */
	    tr = NULL;
	}
    }

    return 0;
    AROS_USERFUNC_EXIT
}
	

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/* HISTORY:  28.01.2001  SDuvan  --  Implemented */


#include <aros/debug.h>
#include <aros/asmcall.h>
#include <dos/dos.h>
#include <proto/realtime.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <exec/lists.h>
#include <exec/tasks.h>
#include <libraries/realtime.h>

#include "realtime_intern.h"

/* This should perhaps run as a task with a very high priority (as we must
   use semaphores) and which is signalled by the timer interrupt */

AROS_UFH3(void, Pulse,
	  AROS_UFHA(STRPTR,            argPtr,  A0),
	  AROS_UFHA(ULONG,             argSize, D0),
	  AROS_UFHA(struct ExecBase *, sysBase, A6))
{
    AROS_USERFUNC_INIT

    struct internal_RealTimeBase *RealTimeBase = GPB(FindTask(NULL)->tc_UserData);

    struct Conductor *conductor;
    struct Player    *player;
    struct pmTime     timeMsg;

    APTR lock;

    kprintf("Pulse task entered (%p)\n", FindTask(NULL));

    while (TRUE)
    {
	ULONG  signals;
	ULONG  time;

	signals = Wait(SIGF_SINGLE | SIGBREAKF_CTRL_C);

	if (signals & SIGBREAKF_CTRL_C)
	{
	    /* Shut us down */
	    break;
	}

	/* It was SIGF_SINGLE */

	/* NOTE! 12 should be 1, but as the timing source is the VBlank
	         running at 50Hz instead of the real heartbeat running at
		 600 Hz, we scale things up
		 -The hartbeat is actually supposed to be 1200 Hz, so I
		  changed 12 up to 24. -ksvalast.
		 */
	GPB(RealTimeBase)->rtb_Time += 24;  /* Not sure about that frac time... maybe
					 to take care of other sync sources
					 (not external) whose heartbeats
					 aliases against 600Hz? */
	time = GPB(RealTimeBase)->rtb_Time;

	timeMsg.pmt_Method = PM_TICK;
	timeMsg.pmt_Time = time;

	lock = LockRealTime(RT_CONDUCTORS);

	ForeachNode((struct List *)&GPB(RealTimeBase)->rtb_ConductorList,
		    (struct Node *)conductor)
	{
	    if (conductor->cdt_State == CONDSTATE_RUNNING)
	    {
		// kprintf("Found running conductor '%s'\n",
		//	   conductor->cdt_Link.ln_Name);

		if (conductor->cdt_Flags & CONDUCTF_EXTERNAL)
		{
		    if (!conductor->cdt_Flags & CONDUCTF_GOTTICK)
		    {
			continue;
		    }

		    /* TODO: Fix wrap-around calculations */
		    
		    /* This conductor has an external sync source */
		    if (time < conductor->cdt_ExternalTime)
		    {
			/* Align the time to the external source */
			conductor->cdt_ClockTime = conductor->cdt_ExternalTime;
		    }
		    else if(time > conductor->cdt_MaxExternalTime)
		    {
			/* This time was not within the constraints, so we
			   cannot report it -- we have to wait for the
			   external synchronizer to call ExternalSync()
			   with new values. Note that the conducor's time
			   is not incremented in this case. */
			continue;
		    }
		}
		else
		{
		    /* This conductor doesn't have an external sync source.
		       Increment conductor time */
		    conductor->cdt_ClockTime+=24;
		}
		
		conductor->cdt_Flags &= ~CONDUCTF_METROSET;
		
		ForeachNode((struct List *)&conductor->cdt_Players,
			    (struct Node *)player)
		{
		    // kprintf("Found player '%s' ready = %i, quiet = %i\n", 
		    //	       player->pl_Link.ln_Name,
		    //	       player->pl_Flags & PLAYERF_READY ? 1 : 0,
		    //         player->pl_Flags & PLAYERF_QUIET ? 1 : 0);

		    if (!(player->pl_Flags & PLAYERF_QUIET) &&
			player->pl_Flags & PLAYERF_READY)
		    {
			// kprintf("Found active player\n");

			if (player->pl_Hook != NULL)
			{
			    // kprintf("Calling hook %p\n", player->pl_Hook);
			    CallHookA(player->pl_Hook, &timeMsg, player);
			}
			else
			{
			    player->pl_MetricTime = conductor->cdt_ClockTime;
			}
			
			/* Alarm check */
			if (player->pl_Flags & PLAYERF_ALARMSET)
			{
			    if (player->pl_AlarmTime <= player->pl_MetricTime)
			    {
				/* AlarmSigBit */
				if (player->pl_Reserved0 != -1)
				{
				    Signal(player->pl_Task,
					   1 << player->pl_Reserved0);
				}
			    }
			}
		    }
		} /* For each active player */
	    }
	} /* For each conductor whose clock is running */
	
	UnlockRealTime(lock);
    } /* forever */

    AROS_USERFUNC_EXIT
}

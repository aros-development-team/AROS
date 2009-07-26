/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Wait CLI Command.
*/

/******************************************************************************


    NAME

        Wait [(n)] [SEC | SECS | MIN | MINS] [ UNTIL (time) ]

    SYNOPSIS

        TIME/N,SEC=SECS/S,MIN=MINS/S,UNTIL/K

    LOCATION

        C:

    FUNCTION

        Wait a certain amount of time or until a specified time. Using
        Wait without any arguments waits for one second.
   
    INPUTS

        TIME      --  the number of time units to wait (default is seconds)
	SEC=SECS  --  set the time unit to seconds
	MIN=MINS  --  set the time unit to minutes
	UNTIL     --  wait until the specified time is reached. The time
                      is given in the format HH:MM.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <exec/execbase.h>
#include <exec/libraries.h>
#include <devices/timer.h>
#include <dos/dos.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>
#include <stdio.h>

const TEXT version[] = "$VER: Wait 41.2 (30.4.2000)\n";

int __nocommandline;

int main (void)
{
    IPTR 		args[4] = { 0, 0, 0, 0 };
    struct RDArgs 	*rda;
    LONG 		error = RETURN_OK;
    ULONG 		delay = 1;
    
#define ERROR(a) { error = a; goto end; }

    rda = ReadArgs("TIME/N,SEC=SECS/S,MIN=MINS/S,UNTIL/K", args, NULL);

    if (rda == NULL)
    {
	PrintFault(IoErr(),"Wait");
        ERROR(RETURN_FAIL);
    }
    
    if (args[3])
    {
        /* UNTIL */
	struct DateTime  	dt;
	struct DateStamp 	ds;
	LONG 			now_secs, then_secs, diff_secs;
	UBYTE			timestring[9];

	DateStamp(&ds);	
	now_secs = ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND;

	if (strlen((char *)args[3]) > 5)
	{
	    puts("Time should be HH:MM");
	    ERROR(RETURN_FAIL);
	}

	strcpy(timestring, (UBYTE *)args[3]);
	strcat(timestring, ":00");
		
	memset(&dt, 0, sizeof(dt));	
	dt.dat_StrTime = timestring;

	if (!StrToDate(&dt))
	{
	    puts("Time should be HH:MM");
	    ERROR(RETURN_FAIL);
	}
	
	then_secs = dt.dat_Stamp.ds_Minute * 60 + dt.dat_Stamp.ds_Tick / TICKS_PER_SECOND;
	diff_secs = then_secs - now_secs;

	if (diff_secs < 0)
	{
	    diff_secs += 60L * 60L * 24L;
	}

	delay = diff_secs * TICKS_PER_SECOND;

    }
    else
    {
	if (args[0])
	{
	    delay = *((ULONG *)args[0]);
	}

	if (args[2])
	{
	    delay *= 60L;
	}
	    
	delay *= TICKS_PER_SECOND;
    }

    if (delay > 0)
    {
	if (delay <= TICKS_PER_SECOND)
	{
	    /* Don't care about breaking if delay is less than 1 second */
	    Delay (delay);
	}
	else
	{
	    struct MsgPort 	*timermp;
	    struct timerequest  *timerio;
	    BOOL		memok = FALSE, devok = FALSE;

	    if ((timermp = CreateMsgPort()))
	    {
	        if ((timerio = (struct timerequest *)CreateIORequest(timermp, sizeof(struct timerequest))))
		{
		    memok = TRUE;
		    if (OpenDevice("timer.device", UNIT_VBLANK, &timerio->tr_node, 0) == 0)
		    {
		        ULONG timermask, sigs;
			BOOL  done = FALSE;
			
		        devok = TRUE;
			
        		timerio->tr_node.io_Command = TR_ADDREQUEST;
			timerio->tr_time.tv_secs    = delay / TICKS_PER_SECOND;
			timerio->tr_time.tv_micro   = 1000000UL / TICKS_PER_SECOND * (delay % TICKS_PER_SECOND);
			
			timermask = 1L << timermp->mp_SigBit;
			
			SendIO(&timerio->tr_node);
			
			while(!done)
			{
			    sigs = Wait(SIGBREAKF_CTRL_C | timermask);

			    if (sigs & timermask)
			    {
			        done = TRUE;
			    }

			    if (sigs & SIGBREAKF_CTRL_C)
			    {
			        if (!CheckIO(&timerio->tr_node)) AbortIO(&timerio->tr_node);
				WaitIO(&timerio->tr_node);
				
				error = RETURN_WARN;				
				done = TRUE;
			    }
			    
			} /* while(!finished) */
			CloseDevice(&timerio->tr_node);
			
		    } /* if (OpenDevice("timer.device", UNIT_VBLANK, &timerio->tr_node, 0) == 0) */
		    DeleteIORequest(&timerio->tr_node);
		    
		} /* if (timerio = (struct timerequest *)CreateIORequest(timermp, sizeof(struct timerequest))) */
		DeleteMsgPort(timermp);
		
	    } /* if ((timermp = CreateMsgPort())) */
	    
	    if (!memok)
	    {
	    	PrintFault(ERROR_NO_FREE_STORE,"Wait");
		ERROR(RETURN_FAIL);
	    }
	    else if (!devok)
	    {
	        puts("Wait: Could not open timer.device!");
		ERROR(RETURN_FAIL);
	    }
	    
	}
	
    } /* if (delay > 0) */

end:
    if (rda)
    {
	FreeArgs(rda);
    }

    return error;
}

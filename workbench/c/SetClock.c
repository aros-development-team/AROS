/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SetClock - set/save the date from/to the BBU clock.
    Lang: English
*/

/*************************************************************************

    NAME
	SetClock

    FORMAT
	SetClock {LOAD|SAVE|RESET}

    SYNOPSIS
	LOAD/S,SAVE/S,RESET/S

    LOCATION
	C:

    FUNCTION
	SetClock can be used to:
	    o Load the time from the battery backed-up clock,
	    o Save the time to the battery backed-up clock,
	    o Reset the battery backed up clock.

    EXAMPLE

	SetClock LOAD

	    will set the system time from the battery backed-up clock.
	    In most systems this will be done automatically during
	    system startup.

	SetClock SAVE

	    will set the time of the battery backed-up clock from the
	    current system clock time.

	SetClock RESET

	    will reset the battery backed-up to a value of the
	    1st January 1978 00:00:00. This is mostly used if the
	    battery backed-up clock has an error and will not
	    respond to normal load and save commands.

    SEE ALSO
	Date, Sys:Prefs/Time

*************************************************************************/

#include <aros/debug.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <devices/timer.h>
#include <libraries/locale.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>
#include <proto/locale.h>
#include <proto/timer.h>

const char version[] = "$VER: SetClock 42.0 (" ADATE ")";
const char exthelp[] =
    "SetClock : Set or save the date from/to the battery backed-up clock\n"
    "\tLOAD     Load the time from the battery-backed-up clock\n"
    "\tSAVE     Save the time to the battery-backed-up clock\n"
    "\tRESET    Reset the battery-backed-up clock\n";

#define ARG_TEMPLATE    "LOAD/S,SAVE/S,RESET/S"
#define ARG_LOAD        0
#define ARG_SAVE        1
#define ARG_RESET       2
#define TOTAL_ARGS	3

int main(int argc, char **av)
{
    IPTR args[TOTAL_ARGS] = { 0, 0, 0 };
    struct RDArgs *rda, *rd;
    struct Library *BattClockBase = NULL;
    struct Device *TimerBase = NULL;
    ULONG time, error = 0;
    struct timerequest *tr;
    struct MsgPort *mp;
    struct timeval t;

    rda = AllocDosObject(DOS_RDARGS, NULL);
    if(rda != NULL)
    {
	rda->RDA_ExtHelp = (STRPTR)exthelp;
	rd = ReadArgs(ARG_TEMPLATE, args, rda);

	if(rd)
	{
	    BattClockBase = OpenResource("battclock.resource");
	    if(BattClockBase)
	    {
		if((mp = CreateMsgPort()))
		{
		    if((tr = (struct timerequest *)
			    CreateIORequest(mp, sizeof(struct timerequest))))
		    {
			if(OpenDevice("timer.device", UNIT_VBLANK,
				(struct IORequest *)tr, 0) == 0)
			{
			    struct Locale *l;
			    struct Process *me = (struct Process *)FindTask(NULL);

			    /* Suppress 'Please insert volume' requesters because we can run without ENV: assign */
			    me->pr_WindowPtr = (APTR)-1;

			    /*
			     * Open default preferences manually because we run before IPrefs,
			     * and default locale isn't set yet.
			     */
			    l = OpenLocale("ENV:SYS/locale.prefs");
			    if (!l)
			    	l = OpenLocale("SYS:Prefs/Env-Archive/SYS/locale.prefs");

			    D(Printf("Preferences locale: 0x%p\n", l));

			    TimerBase = tr->tr_node.io_Device;

			    if(args[0])
			    {
				/* Loading */
				time = ReadBattClock();

				if (l)
				{
				    D(Printf("Locale flags 0x%08lx, offset %ld\n", l->loc_Flags, l->loc_GMTOffset));

				    if (l->loc_Flags & LOCF_GMT_CLOCK)
				    {
				    	/* loc_GMTOffset actually expresses difference from local time to GMT */
				    	time -= l->loc_GMTOffset * 60;
				    }
				}

				/* Set timer.device clock */
				tr->tr_node.io_Command = TR_SETSYSTIME;
				tr->tr_time.tv_secs = time;
				tr->tr_time.tv_micro = 0;
				tr->tr_node.io_Flags = IOF_QUICK;
				DoIO((struct IORequest *)tr);
				if(tr->tr_node.io_Error != 0)
				{
				    FPuts(Output(), "Error: Could not set system time!\n");
				}
			    }
			    else if(args[1])
			    {
				/* Saving */
				GetSysTime(&t);

				if (l)
				{
				    if (l->loc_Flags & LOCF_GMT_CLOCK)
				    	t.tv_secs += l->loc_GMTOffset * 60;
				}
				WriteBattClock(t.tv_secs);
			    }
			    else if(args[2])
			    {
				/* Resetting */
				ResetBattClock();
			    }
			    else
				error = ERROR_REQUIRED_ARG_MISSING;

			    if (l)
			    	CloseLocale(l);

			} /* OpenDevice() */
			else
			    error = ERROR_INVALID_RESIDENT_LIBRARY;

			DeleteIORequest((struct IORequest *)tr);

		    } /* CreateIORequest() */
		    else
			error = ERROR_NO_FREE_STORE;

		    DeleteMsgPort(mp);
		} /* CreateMsgPort() */

		else
		    error = ERROR_NO_FREE_STORE;

	    } /* OpenResource() */
	    FreeArgs(rd);

	} /* ReadArgs() */
	else
	    error = IoErr();

	FreeDosObject(DOS_RDARGS, rda);
    } /* AllocDosObject() */
    else
	error = IoErr();

    if(error != 0)
    {
	PrintFault(error, "SetClock");
	return RETURN_FAIL;
    }
    SetIoErr(0);
    return 0;
}

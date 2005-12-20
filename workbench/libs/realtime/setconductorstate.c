/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/realtime.h>
#include <exec/lists.h>
#include "realtime_intern.h"

/*****************************************************************************

    NAME */
#include <libraries/realtime.h>

    AROS_LH3(LONG, SetConductorState,

/*  SYNOPSIS */

	AROS_LHA(struct Player *, player, A0),
	AROS_LHA(ULONG          , state , D0),
	AROS_LHA(LONG           , time  , D1),

/*  LOCATION */

	struct Library *, RealTimeBase, 10, RealTime)

/*  FUNCTION

    Changes the state of the conductor connected to a specified player.
    The possible states are
    
    CONDSTATE_STOPPED
    CONDSTATE_PAUSED
    CONDSTATE_LOCATE
    CONDSTATE_RUNNING

    other possible "states" are

    CONDSTATE_METRIC   --  Ask the highest priority conducted node to do a
                           CONDSTATE_LOCATE
    CONDSTATE_SHUTTLE  --  Inform the players that the clock value is
                           changing without the clock running


    INPUTS

    player   --  The player in question
    state    --  The new state of the conductor
    time     --  Start time offset in realtime.library units

    RESULT

    0 if OK, otherwise an error code. For now, these are RTE_PLAYING and
    RTE_NOCONDUCTOR.

    NOTES

    Going from CONDSTATE_PAUSED to CONDSTATE_RUNNING does not reset the
    cdt_ClockTime of the conductor.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    27.7.1999  SDuvan  implemented parts
    27.1.2001  SDuvan  implemented the rest

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct pmState    stateMsg = { PM_STATE, 0 };
    struct pmTime     timeMsg  = { 0 , time };
    struct Player    *pl;    /* For use in ForeachNode() */
    struct Conductor *conductor = player->pl_Source;

    if (conductor == NULL)
    {
	return RTE_NOCONDUCTOR;
    }

    stateMsg.pms_OldState = conductor->cdt_State;

    /* Don't report phony states */
    if (state >= 0)
    {
	ForeachNode(&conductor->cdt_Players, pl)
	{
	    /* Filter out QUIET players? */
	    if (pl->pl_Hook != NULL)
	    {
		CallHookA(pl->pl_Hook, &stateMsg, player);
	    }
	}
    }
    
    switch (state)
    {
    case CONDSTATE_PAUSED:
	
	/* Pause the clock */
	conductor->cdt_State = CONDSTATE_PAUSED;
	conductor->cdt_Flags &= ~CONDUCTF_GOTTICK;

	break;

    case CONDSTATE_STOPPED:

	/* Stop the clock */
	conductor->cdt_State = CONDSTATE_STOPPED;
	conductor->cdt_Flags &= ~CONDUCTF_GOTTICK;

	break;

    case CONDSTATE_LOCATE:
	{
	    ULONG oldSignals;

	    conductor->cdt_State = CONDSTATE_LOCATE;

	    ObtainSemaphore(&conductor->cdt_Lock);
	    oldSignals = SetSignal(0, SIGF_SINGLE);
	    conductor->cdt_Barrier = FindTask(NULL);
	    ReleaseSemaphore(&conductor->cdt_Lock);

	    ForeachNode(&conductor->cdt_Players, pl)
	    {
		BOOL isReady = FALSE;

		/* Barrier synchronization */
		while (!isReady)
		{
		    struct TagItem tags[] = { { PLAYER_Ready, (IPTR)&isReady },
					      { TAG_DONE    , (IPTR)NULL   } };

		    GetPlayerAttrsA(pl, tags);

		    if (!isReady)
		    {
			/* We are signalled by SetPlayerAttrs() if the tags
			   contain PLAYER_Ready '=' TRUE) */
			Wait(SIGF_SINGLE);
		    }
		}
	    }

	    ObtainSemaphore(&conductor->cdt_Lock);
	    conductor->cdt_Barrier = NULL;
	    SetSignal(oldSignals, SIGF_SINGLE);
	    ReleaseSemaphore(&conductor->cdt_Lock);
	}

	/* Send PM_STATE message with CONDSTATE_LOCATE_SET here? */

	/* Fall through */
	
    case CONDSTATE_RUNNING:
	/* Start clock */
	conductor->cdt_ClockTime = time;
	conductor->cdt_State = CONDSTATE_RUNNING;

	break;

    case CONDSTATE_METRIC:
	{
	    /* Get the highest priority musically aware player and let him
	       take care of the time location process. He will later call
	       SetConductorState() with state CONDSTATE_LOCATE to set the
	       time in realtime units calculated from his internal awareness
	       of time. */
	    struct Player *maestro = NULL;

	    ForeachNode(&conductor->cdt_Players,pl)
	    {
		if (pl->pl_Flags & PLAYERF_CONDUCTED)
		{
		    maestro = pl;
		    break;
		}
	    }

	    if (maestro == NULL)
	    {
		/* There is no defined error to return in this situation */
	    }
	    else
	    {
		timeMsg.pmt_Method = PM_POSITION;
		CallHookA(maestro->pl_Hook, &timeMsg, player);
	    }
	    
	    break;
	}

    case CONDSTATE_SHUTTLE:
	/* Shuttling not allowed when playing */
	if (conductor->cdt_State == CONDSTATE_RUNNING)
	{
	    return RTE_PLAYING;
	}

	conductor->cdt_StartTime = time;

	timeMsg.pmt_Method = PM_SHUTTLE;
	
	ForeachNode(&conductor->cdt_Players, pl)
	{
	    if (pl->pl_Hook != NULL)
	    {
		CallHookA(pl->pl_Hook, &timeMsg, player);
	    }
	}
	
	break;
    }

    return 0;			/* Success */

    AROS_LIBFUNC_EXIT
} /* SetConductorState */

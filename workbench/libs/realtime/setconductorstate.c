
/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#ifndef AROS_ALMOST_COMPATIBLE
#define AROS_ALMOST_COMPATIBLE
#endif

#include <proto/exec.h>
#include <proto/alib.h>
#include <libraries/realtime.h>
#include <exec/lists.h>

    AROS_LH3(LONG, SetConductorState,

/*  SYNOPSIS */

	AROS_LHA(struct Player *, player, A0),
	AROS_LHA(ULONG          , state , D0),
	AROS_LHA(LONG           , time  , D1),

/*  LOCATION */

	struct Library *, RTBase, 10, RealTime)

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

    27.7.99  SDuvan  implemented parts

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct pmState stateMsg = { PM_STATE, 0 };
    struct pmTime  timeMsg  = { 0 , time };
    struct Node   *tempNode;    /* For use in ForeachNode() */
    APTR           lock;	/* Lock on RT_CONDUCTORS */

    if(player->pl_Source == NULL)
	return RTE_NOCONDUCTOR;

    stateMsg.pms_OldState = player->pl_Source->cdt_State;

    /* Don't report phony states */
    if(state >= 0)
    {
	ForeachNode((struct List *)&player->pl_Source->cdt_Players, tempNode)
	{
	    if(((struct Player *)tempNode)->pl_Hook != NULL)
		CallHookA(((struct Player *)tempNode)->pl_Hook,
			  &stateMsg, player);
	}
    }

    /* What does LOCATE really mean? */
    switch(state)
    {
    case CONDSTATE_STOPPED:
	player->pl_Source->cdt_ClockTime = 0;

	/* Fall through */
	
    case CONDSTATE_PAUSED:
	/* Pause clock */
	break;

    case CONDSTATE_RUNNING:
	/* Start clock */
	break;

    case CONDSTATE_LOCATE:
	player->pl_Source->cdt_ClockTime = time;
	break;

    case CONDSTATE_METRIC:
	lock = LockRealTime(RT_CONDUCTORS);
	
	/* Find the player with highest priority (TODO) and. */

	UnlockRealTime(lock);
	break;

    case CONDSTATE_SHUTTLE:
	/* Shuttling not allowed when playing */
	if(player->pl_Source->cdt_State == CONDSTATE_RUNNING)
	    return RTE_PLAYING;

	player->pl_Source->cdt_StartTime = time;

	timeMsg.pmt_Method = PM_SHUTTLE;
	
	ForeachNode((struct List *)&player->pl_Source->cdt_Players, tempNode)
	{
	    if(((struct Player *)tempNode)->pl_Hook != NULL)
		CallHookA(((struct Player *)tempNode)->pl_Hook,
			  &timeMsg, player);
	}

	break;
    }

    return 0;			/* Success */

    AROS_LIBFUNC_EXIT
} /* SetConductorState */

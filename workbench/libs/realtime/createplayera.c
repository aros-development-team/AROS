/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/
# define  DEBUG 1
# include <aros/debug.h>

#include <proto/exec.h>
#include <proto/realtime.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include "realtime_intern.h"

struct Conductor *createConductor(BOOL private, LONG *error,STRPTR name,
				  struct Library *RealTimeBase);

/*****************************************************************************

    NAME */
#include <libraries/realtime.h>

    AROS_LH1(struct Player *, CreatePlayerA,

/*  SYNOPSIS */

	AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */

	struct Library *, RealTimeBase, 7, RealTime)

/*  FUNCTION

    Create a player.

    INPUTS

    tagList  --  pointer to an array of tags describing the player's
                 attributes or NULL.

    TAGS

    PLAYER_Name (STRPTR)         --  The name of the player; default is no
                                     name.

    PLAYER_Hook (struct Hook *)  --  Function to call every time the time
                                     changes; default is none. The hook is
				     called with

				     a0 -- address of Hook structure
				     a1 -- message (see <libraries/realtime.h>)
				     a2 -- address of Player structure

				     Be aware of that the function is not
				     necessarily called TICK_FREQ times per
				     second: this is the upper limit of times
				     it may be called.

    PLAYER_Priority (BYTE)       --  The priority of the player; default is 0.

    PLAYER_Conductor (STRPTR)    --  The name of the conductor to link the
                                     player to. If the conductor doesn't exist,
				     it's created automatically. Passing ~0
				     creates a private conductor.

    PLAYER_Ready (BOOL)          --  Set / clear the ready flag; default is
                                     TRUE.

    PLAYER_AlarmTime (LONG)      --  Set player's alarm time; implies setting
                                     the PLAYERF_ALARMSET flag.

    PLAYER_Alarm (BOOL)          --  Set / clear the PLAYERF_ALARMSET flag;
                                     default is FALSE.

    PLAYER_AlarmSigTask (struct Task *)
                                 --  The task to signal when the alarm goes
				     off; default is no task. If no task is
				     specified PLAYERF_ALARMSET is turned
				     off.

    PLAYER_AlarmSigBit (BYTE)    --  Signal bit to use for the alarm or -1
                                     to disable signalling; default is -1.

    PLAYER_Quiet (BOOL)          --  Specify whether this player should be
                                     ignored or not; default is FALSE.
				     Generally only used by external sync
				     applications.
				     
    PLAYER_UserData (VOID *)     --  Set pointer to user specific data;
                                     default is NULL.

    PLAYER_ID (UWORD)            --  Set the player's ID; default is 0.

    PLAYER_Conducted (BOOL)      --  Set / clear the PLAYERF_CONDUCTED flag;
                                     default is FALSE.

    PLAYER_ExtSync (BOOL)        --  If TRUE, this player attempts to become
                                     the external sync source.

    PLAYER_ErrorCode (LONG *)    --  Optional pointer to a LONG that will
                                     contain an error code if the function
				     fails. Possible error values are:

				     RTE_NOMEMORY  --  memory allocation failed
				     RTE_NOTIMER   --  timer allocation failed

    RESULT

    A pointer to a player structure or NULL if failure. In case of a failure
    additional information may be retreived from the LONG variable pointed
    to by PLAYER_ErrorCode if you have specified that tag.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DeletePlayer(), GetPlayerAttrs(), SetPlayerAttrs()

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tl = tagList;
    struct Player  *player = AllocMem(sizeof(struct Player), 
				      MEMF_PUBLIC | MEMF_CLEAR);
    LONG           *error;

    D(bug("Entering CreatePlayerA()\n"));

    error = (LONG *) GetTagData(PLAYER_ErrorCode, (IPTR) NULL, tl);

    if (player == NULL)
    {
	if (error != NULL)
	{
	    *error = RTE_NOMEMORY;
	}

	return NULL;
    }

    /* Set default values */
    player->pl_Reserved0 = -1;	          /* AlarmSigBit */
    player->pl_Flags |= PLAYERF_READY;

    while((tag = NextTagItem(&tl)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	case PLAYER_Conductor:

	    D(bug("Found PLAYER_Conductor tag\n"));

	    if ((IPTR)tag->ti_Data == -1)
	    {
		player->pl_Source = createConductor(TRUE, error,
						    (STRPTR)tag->ti_Data,
						    RealTimeBase);
	    }
	    else
	    {
		struct Conductor *cd = FindConductor((STRPTR)tag->ti_Data);

		if (cd == NULL)
		{
		    D(bug("Trying to create a public conductor.\n"));
		    player->pl_Source = createConductor(FALSE, error,
							(STRPTR)tag->ti_Data,
							RealTimeBase);
		}
		else
		{
		    player->pl_Source = cd;
		}
	    }

	    if (player->pl_Source != NULL)
	    {
		APTR lock;

		lock = LockRealTime(RT_CONDUCTORS);
		
		/* Enqueue the player to the conductor list */
		Enqueue((struct List *)&player->pl_Source->cdt_Players,
			(struct Node *)player);
		
		UnlockRealTime(lock);
	    }

	    break;
	}

	/* The rest of the tags are taken care of in SetPlayerAttrsA() */
    }

    D(bug("Calling SetPlayerAttrsA()\n"));

    if (SetPlayerAttrsA(player, tagList))
    {
	return player;
    }
    else
    {
	return NULL;
    }

    AROS_LIBFUNC_EXIT
} /* CreatePlayerA */


struct Conductor *createConductor(BOOL private, LONG *error, STRPTR name,
				  struct Library *RealTimeBase)
{
    struct Conductor *cd = AllocMem(sizeof(struct Conductor), 
				    MEMF_PUBLIC | MEMF_CLEAR);

    if (cd == NULL)
    {
	if (error != NULL)
	{
	    *error = RTE_NOMEMORY;
	}

	return NULL;
    }

    cd->cdt_Link.ln_Name = name;

    NEWLIST(&cd->cdt_Players);
    InitSemaphore(&cd->cdt_Lock);

    /* Initialize conductor clock */
    cd->cdt_ClockTime = GPB(RealTimeBase)->rtb_Time;
    cd->cdt_StartTime = GPB(RealTimeBase)->rtb_Time;

    /* Conductors are created in 'stopped' mode. To make the clock start
       running, call SetConductorState(player, CONDSTATE_RUNNING, _); */
    cd->cdt_State = CONDSTATE_STOPPED;

    if (private)
    {
	cd->cdt_Flags |= CONDUCTF_PRIVATE;
    }

    {
	/* Add the conductor to the realtime library conductor list */
	APTR lock;

	lock = LockRealTime(RT_CONDUCTORS);
	
	AddTail((struct List *)&GPB(RealTimeBase)->rtb_ConductorList,
		(struct Node *)cd);
	
	UnlockRealTime(lock);
    }	

    return cd;
}

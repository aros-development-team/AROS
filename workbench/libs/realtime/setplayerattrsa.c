
/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include <proto/exec.h>
#include <proto/realtime.h>
#include <exec/lists.h>
#include <utility/tagitem.h>

    AROS_LH2(BOOL, SetPlayerAttrsA,

/*  SYNOPSIS */

	AROS_LHA(struct Player  *, player , A0), 
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */

	struct Library *, RTBase, 9, RealTime)

/*  FUNCTION

    Sets the attributes of a player. An attribute not specified in the array
    of tags is unchanged after this procedure.

    INPUTS

    player   --  The player the attributes of which to set.
    tagList  --  Pointer to an array of tags describing the player's
                 attributes or NULL.

    TAGS

    The same tags as for CreatePlayerA().

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

    HISTORY

    26.7.99  SDuvan  implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    LONG           *error = NULL;
    struct TagItem *tag, *tl = tagList;


    error = (LONG *)GetTagData(PLAYER_ErrorCode, NULL, tl);

    while ((tag = NextTagItem(&tl)) != NULL)
    {
	switch(tag->ti_Tag)
	{
	case PLAYER_Name:
	    player->pl_Link.ln_Name = (APTR)tag->ti_Data;
	    break;

	case PLAYER_Hook:
	    player->pl_Hook = (struct Hook *)tag->ti_Data;
	    break;

	case PLAYER_Priority:
	    player->pl_Link.ln_Pri = (BYTE)tag->ti_Data;

	    if(player->pl_Link.ln_Succ != NULL)
	    {
		/* If this node has been (is) inserted before, then remove it
		   and put it in the right place. */

		/* Is this player attached to a conductor? */
		if(player->pl_Source != NULL)
		{		    
		    Remove((struct Node *)player);
		    Enqueue((struct List *)&player->pl_Source->cdt_Players,
			    (struct Node *)player);
		}
		else
		{
		    if(error != NULL)
			*error = RTE_NOCONDUCTOR;

		    return FALSE;
		}
	    }

	    break;

	case PLAYER_Conductor:
	    if(tag->ti_Data == NULL)
		player->pl_Source = NULL;
	    else
	    {
		struct Conductor *conductor;
		APTR              lock;

		lock = LockRealTime(RT_CONDUCTORS);
		conductor = FindConductor((STRPTR)tag->ti_Data);
		UnlockRealTime(lock);

		if(conductor == NULL)
		{
		    if(error != NULL)
			*error = RTE_NOCONDUCTOR;

		    return FALSE;
		}
	    }

	    break;

	case PLAYER_Ready:
	    if((BOOL)tag->ti_Data)
		player->pl_Flags |= PLAYERF_READY;
	    else
		player->pl_Flags &= ~PLAYERF_READY;
		
	    break;

	case PLAYER_AlarmTime:
	    player->pl_Flags |= PLAYERF_ALARMSET;
	    player->pl_AlarmTime = (LONG)tag->ti_Data;
	    break;

	case PLAYER_Alarm:
	    if((BOOL)tag->ti_Data)
		player->pl_Flags |= PLAYERF_ALARMSET;
	    else
		player->pl_Flags &= ~PLAYERF_ALARMSET;

	    break;

	case PLAYER_AlarmSigTask:
	    if((struct Task *)tag->ti_Data == NULL)
		player->pl_Flags &= ~PLAYERF_ALARMSET;
		
	    player->pl_Task = (struct Task *)tag->ti_Data;
	    break;

	case PLAYER_AlarmSigBit:
	    if((BYTE)tag->ti_Data == -1)
		player->pl_Flags &= ~PLAYERF_ALARMSET;

	    /* We could use player->pl_Link.ln_Type here */
	    player->pl_Reserved0 = (BYTE)tag->ti_Data;      /* NOTE! */
	    break;

	case PLAYER_Quiet:
	    if((BOOL)tag->ti_Data)
		player->pl_Flags |= PLAYERF_QUIET;
	    else
		player->pl_Flags &= ~PLAYERF_QUIET;
		
	    break;

	case PLAYER_UserData:
	    player->pl_UserData = (APTR)tag->ti_Data;
	    break;

	case PLAYER_ID:
	    player->pl_PlayerID = (UWORD)tag->ti_Data;
	    break;

	case PLAYER_Conducted:
	    if((BOOL)tag->ti_Data)
		player->pl_Flags |= PLAYERF_CONDUCTED;
	    else
		player->pl_Flags &= ~PLAYERF_CONDUCTED;

	    break;

	case PLAYER_ExtSync:
	    /* TODO: Try to become the external sync source */

	    if((BOOL)tag->ti_Data)
	    {
		BOOL succeed;
		/* Try to become external sync source */

		if(succeed)
		    player->pl_Flags |= PLAYERF_EXTSYNC;
	    }
	    else
		player->pl_Flags &= ~PLAYERF_EXTSYNC;

	    break;
	}
    }

    /* Consistency checks */
    if(player->pl_Task == NULL)
	player->pl_Flags &= ~PLAYERF_ALARMSET;


    return TRUE;

    AROS_LIBFUNC_EXIT
} /* SetPlayerAttrsA */

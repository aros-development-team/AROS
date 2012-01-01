/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/realtime.h>
#include <proto/utility.h>
#include "realtime_intern.h"

/*****************************************************************************

    NAME */

#include <utility/tagitem.h>
#include <libraries/realtime.h>

    AROS_LH2(BOOL, GetPlayerAttrsA,

/*  SYNOPSIS */

	AROS_LHA(struct Player  *, player , A0), 
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */

	struct Library *, RealTimeBase, 14, RealTime)

/*  FUNCTION

    Query the attributes of a player. For each tagitem ti_Tag specifies the 
    attribute and ti_Data a pointer to the IPTR variable in which you want
    the value to be stored.

    INPUTS

    player   --  The player the attributes of which to set; may be NULL,
                 in which case the result is 0.
    tagList  --  Pointer to an array of tags describing the player's
                 attributes or NULL.

    TAGS

    See CreatePlayerA().

    RESULT

    The number of items successfully filled in.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    CreatePlayerA(), SetPlayerAttrsA()

    INTERNALS

******************************************************************************/

#define Put (*((IPTR *)(tag->ti_Data)))

{
    AROS_LIBFUNC_INIT

    int             nAttrs = 0;
    struct TagItem *tl = tagList;
    struct TagItem *tag;

    /* Maybe we could use FindTagItem to initialize error and conductor
       first... */

    if (player != NULL)
    {
	while ((tag = NextTagItem(&tl)) != NULL)
	{
	    switch (tag->ti_Tag)
	    {
	    case PLAYER_Name:
		Put = (IPTR)player->pl_Link.ln_Name;
		break;
		
	    case PLAYER_Hook:
		Put = (IPTR)player->pl_Hook;
		break;
		
	    case PLAYER_Priority:
		Put = (IPTR)player->pl_Link.ln_Pri;
		break;
		
	    case PLAYER_Conductor:
		Put = (IPTR)player->pl_Source;
		break;
		
	    case PLAYER_Ready:
		Put = (player->pl_Flags & PLAYERF_READY) != 0;
		break;
		
	    case PLAYER_AlarmTime:
		Put = player->pl_AlarmTime;
		break;
		
	    case PLAYER_Alarm:
		Put = (player->pl_Flags & PLAYERF_ALARMSET) != 0;
		break;
		
	    case PLAYER_AlarmSigTask:		
		Put = (IPTR)player->pl_Task;
		break;
		
	    case PLAYER_AlarmSigBit:
		/* We could use player->pl_Link.mn_Type here */
		Put = player->pl_Reserved0;    /* NOTE! */
		break;
		
	    case PLAYER_Quiet:
		Put = (player->pl_Flags & PLAYERF_QUIET) != 0;
		break;
		
	    case PLAYER_UserData:
		Put = (IPTR)player->pl_UserData;
		break;
		
	    case PLAYER_ID:
		Put = player->pl_PlayerID;
		break;
		
	    case PLAYER_Conducted:
		Put = (player->pl_Flags & PLAYERF_CONDUCTED) != 0;
		break;
		
	    case PLAYER_ExtSync:
		Put = (player->pl_Flags & PLAYERF_EXTSYNC) != 0;
		break;
	    }
	    
	    nAttrs++;
	}
    }

    return nAttrs;

    AROS_LIBFUNC_EXIT
} /* GetPlayerAttrsA */

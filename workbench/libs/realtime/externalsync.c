/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */

#include <libraries/realtime.h>

    AROS_LH3(BOOL, ExternalSync,

/*  SYNOPSIS */

	AROS_LHA(struct Player *, player , A0),
	AROS_LHA(LONG           , minTime, D0),
	AROS_LHA(LONG           , maxTime, D1),

/*  LOCATION */

	struct Library *, RealTimeBase, 11, RealTime)

/*  FUNCTION

    Constrain conductor time between 'minTime' and 'maxTime' (however, time
    can never run backwards). If the specified player isn't the current
    external synchronizing source, this function does nothing.

    INPUTS

    player   --  The player in question
    minTime  --  Lower time bound
    maxTime  --  Upper time bound

    RESULT

    A BOOL specifying if the success of this function; FALSE means that the
    player was not the external source or that no conductor was found for
    the player.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    27.7.99  SDuvan  implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if ((player == NULL) || (player->pl_Source == NULL) ||
	!(player->pl_Flags & PLAYERF_EXTSYNC))
    {
	return FALSE;
    }

    player->pl_Source->cdt_ExternalTime    = minTime;
    player->pl_Source->cdt_MaxExternalTime = maxTime;
    player->pl_Source->cdt_Flags |= CONDUCTF_GOTTICK;

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* ExternalSync */

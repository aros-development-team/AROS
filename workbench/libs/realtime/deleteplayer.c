/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include <proto/exec.h>
#include <libraries/realtime.h>

    AROS_LH1(VOID, DeletePlayer,

/*  SYNOPSIS */

	AROS_LHA(struct Player *, player, A0),

/*  LOCATION */

	struct Library *, RealTimeBase, 8, RealTime)

/*  FUNCTION

    Delete a player. If this was the last player of a specific conductor,
    this conductor is deleted too.

    INPUTS

    player  --  Player to delete; may be NULL in which case this function
                does nothing.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    CreatePlayerA()

    INTERNALS

    HISTORY

    26.7.99  SDuvan  implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    struct Conductor *conductor;

    if (player == NULL)
    {
	return;
    }

    conductor = player->pl_Source;

    if (conductor != NULL)
    {
	APTR lock;

	lock = LockRealTime(RT_CONDUCTORS);
	Remove((struct Node *)player);

	/* If this was the last player of this conductor, we delete the
	   conductor, too. */
	if (IsListEmpty(&conductor->cdt_Players))
	{
	    Remove((struct Node *)conductor);
	    FreeMem(conductor, sizeof(struct Conductor));
	}

	UnlockRealTime(lock);
    }

    FreeMem(player, sizeof(struct Player));

    AROS_LIBFUNC_EXIT
} /* DeletePlayer */


/*
    (C) 1999 AROS - The Amiga Research OS
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

	struct Library *, RTBase, 8, RealTime)

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

    if(player == NULL)
	return;

    if(player->pl_Source != NULL)
    {
	APTR lock;

	lock = LockRealTime(RT_CONDUCTORS);
	Remove((struct Node *)player);

	/* If this was the last player of this conductor, we delete the
	   conductor, too. */
	if(IsListEmpty(&player->pl_Source->cdt_Players))
	{
	    Remove((struct Node *)player->pl_Source);
	    FreeMem(player->pl_Source, sizeof(struct Conductor));
	}

	UnlockRealTime(lock);
    }

    FreeMem(player, sizeof(struct Player));

    AROS_LIBFUNC_EXIT
} /* DeletePlayer */

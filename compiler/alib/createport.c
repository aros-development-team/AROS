/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: amiga.lib function CreatePort()
    Lang: english
*/

#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <exec/ports.h>
#include <proto/alib.h>

	struct MsgPort * CreatePort (

/*  SYNOPSIS */
	STRPTR name,
	LONG   pri)

/*  FUNCTION
	Allocate and initialize a new Exec message port. You must
	use DeletePort() to get rid of it.

    INPUTS
	name - The name of the new port. The string is not copied
	pri - The priority of the port.

    RESULT
	A pointer to the new message port or NULL if no memory or
	no signal was available.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	DeletePort(), exec.library/CreateMsgPort(), exec.library/DeleteMsgPort()

    INTERNALS

    HISTORY

******************************************************************************/
{
    struct MsgPort * mp;

    mp = CreateMsgPort ();

    if (mp)
    {
	mp->mp_Node.ln_Name = name;
	mp->mp_Node.ln_Pri  = pri;

	if (name)
	    AddPort (mp);
    }

    return mp;
} /* CreatePort */


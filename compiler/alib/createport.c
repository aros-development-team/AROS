/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: amiga.lib function CreatePort()
    Lang: english
*/
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
#include <exec/ports.h>
#include <clib/alib_protos.h>

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
	DeletePort(), CreateMsgPort(), DeleteMsgPort()

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


/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: amiga.lib function DeletePort()
    Lang: english
*/
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <exec/ports.h>
#include <proto/alib.h>

	void DeletePort (

/*  SYNOPSIS */
	struct MsgPort * mp)

/*  FUNCTION
	Free a message port created by CreatePort()

    INPUTS
	mp - The result of CreatePort()

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CreatePort(), CreateMsgPort(), DeleteMsgPort()

    INTERNALS

    HISTORY

******************************************************************************/
{
    if (mp->mp_Node.ln_Name)
	RemPort (mp);

    DeleteMsgPort (mp);
} /* DeletePort */


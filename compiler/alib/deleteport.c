/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: amiga.lib function DeletePort()
    Lang: english
*/
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
#include <exec/ports.h>
#include <clib/alib_protos.h>

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


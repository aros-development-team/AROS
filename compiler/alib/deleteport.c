/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: amiga.lib function DeletePort()
    Lang: english
*/
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
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
    DeleteMsgPort (mp);
} /* DeletePort */


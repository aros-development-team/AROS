/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/08/13 13:56:01  digulla
    Replaced __AROS_LA by __AROS_LHA
    Replaced some __AROS_LH*I by __AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:10  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(struct MsgPort *, FindPort,

/*  SYNOPSIS */
	__AROS_LHA(STRPTR, name,A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 65, Exec)

/*  FUNCTION
	Look for a public messageport by name. This function doesn't
	arbitrate for the port list and must be protected with a Forbid()
	Permit() pair.

    INPUTS
	port - Pointer to NUL terminated C string.

    RESULT
	Pointer to struct MsgPort or NULL if there is no port of that name.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    __AROS_FUNC_INIT

    /* Nothing spectacular - just look for that name. */
    return (struct MsgPort *)FindName(&SysBase->PortList,name);
    __AROS_FUNC_EXIT
} /* FindPort */


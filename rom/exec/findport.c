/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:48  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:01  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
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

	AROS_LH1(struct MsgPort *, FindPort,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name,A1),

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
    AROS_LIBFUNC_INIT

    /* Nothing spectacular - just look for that name. */
    return (struct MsgPort *)FindName(&SysBase->PortList,name);
    AROS_LIBFUNC_EXIT
} /* FindPort */


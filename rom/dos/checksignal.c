/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1997/01/27 00:36:15  ldp
    Polish

    Revision 1.2  1996/12/09 13:53:22  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.1  1996/12/05 15:52:23  aros
    Initial revision


    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(LONG, CheckSignal,

/*  SYNOPSIS */
	AROS_LHA(LONG, mask, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 132, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    LONG rcvd;

    /* Get pointer to current task structure */
    struct Task *me = FindTask(NULL);

    /* Protect the task lists against access by other tasks. */
    Disable();

    /* Get active signals specified in mask */
    rcvd = me->tc_SigRecvd & mask;

    /* And clear them. */
    me->tc_SigRecvd &= ~mask;

    /* All done. */
    Enable();

    return rcvd;
    AROS_LIBFUNC_EXIT
} /* CheckSignal */

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/12/10 13:51:50  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.2  1996/10/24 15:50:54  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/10 13:07:07  digulla
    New function: RawPutChar()


    Desc:
    Lang: english
*/
#include <unistd.h>
#include <fcntl.h>
#include "exec_intern.h"

/*****************************************************************************

    NAME */
#include <exec/semaphores.h>
#include <clib/exec_protos.h>

	AROS_LH1(ULONG, RawPutChar,

/*  SYNOPSIS */
	AROS_LHA(ULONG, character, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 86, Exec)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)

    char c=character;
    write(STDERR_FILENO,&c,1);
    return character;
    AROS_LIBFUNC_EXIT
} /* RawPutChar */


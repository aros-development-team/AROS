/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

	__AROS_LH1(ULONG, RawPutChar,

/*  SYNOPSIS */
	__AROS_LHA(ULONG, character, D0),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct ExecBase *,SysBase)

    char c=character;
    write(STDERR_FILENO,&c,1);
    return character;
    __AROS_FUNC_EXIT
} /* RawPutChar */


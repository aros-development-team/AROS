/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sets the name of the current program.
    Lang: English
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */

#include <proto/dos.h>

	AROS_LH1(BOOL, SetProgramName,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 95, Dos)

/*  FUNCTION
	Sets the name for the current program in the CLI structure. If the
	name doesn't fit the old name is kept and a failure is returned.
	If the current process doesn't have a CLI structure this function
	does nothing.

    INPUTS
	name  --  Name for the current program.

    RESULT
	!= 0 on success, 0 on failure.

    NOTES

    EXAMPLE

    BUGS
	Never copies more than 255 bytes.

    SEE ALSO
	GetProgramName()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    return internal_SetProgramName(Cli(), name, DOSBase) ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* SetProgramName */


BOOL internal_SetProgramName(struct CommandLineInterface *cli, STRPTR name,
			     struct DosLibrary *DOSBase)
{
    STRPTR  s;
    ULONG   namelen;

    if (cli == NULL)
    {
	return FALSE;
    }
    
    s = name;

    while(*s++)
	;

    namelen = s - name - 1;
    
    if (namelen > 255)
    {
	return FALSE;
    }
    
    s = AROS_BSTR_ADDR(cli->cli_CommandName);
    
    AROS_BSTR_setstrlen(cli->cli_CommandName, namelen);
    CopyMem((APTR)name, s, namelen);

    return TRUE;
}

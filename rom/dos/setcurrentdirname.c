/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, SetCurrentDirName,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 93, Dos)

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

    struct CommandLineInterface *cli = NULL;
    STRPTR s;
    ULONG namelen;

    if ((cli = Cli()) == NULL)
	return DOSFALSE;

    s = name;
    while(*s++)
	;
    namelen = s - name - 1;

    if (namelen > 255)
	return DOSFALSE;

    s = AROS_BSTR_ADDR(cli->cli_SetName);

    AROS_BSTR_setstrlen(cli->cli_SetName, namelen);
    CopyMem((APTR)name, s, namelen);

    return DOSTRUE;
    AROS_LIBFUNC_EXIT
} /* SetCurrentDirName */

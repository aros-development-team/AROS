/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, GetPrompt,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, buf, D1),
	AROS_LHA(LONG  , len, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 98, Dos)

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

    struct Process *me = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli = BADDR(me->pr_CLI);
    STRPTR cname;
    ULONG clen;
    BOOL ret = DOSTRUE;

    if (cli == NULL)
    {
	if (len >= 1)
	    buf[0] = '\0';
	me->pr_Result2 = ERROR_OBJECT_WRONG_TYPE;
	return DOSFALSE;
    }

    cname = AROS_BSTR_ADDR(cli->cli_Prompt);
    clen = (ULONG)AROS_BSTR_strlen(cli->cli_Prompt);
    if (clen >= (len-1))
    {
	clen = len-1;
	me->pr_Result2 = ERROR_LINE_TOO_LONG;
	ret = DOSFALSE;
    }
    CopyMem(cname, buf, clen);
    buf[clen] = '\0';

    return ret;
    AROS_LIBFUNC_EXIT
} /* GetPrompt */

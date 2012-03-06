/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get the name of the current directory.
    Lang: english
*/
#include <aros/debug.h>

#include <proto/exec.h>
#include <dos/dos.h>
#include "dos_intern.h"
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, GetCurrentDirName,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, buf, D1),
	AROS_LHA(LONG  , len, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 94, Dos)

/*  FUNCTION
	Copies the name of the current directory from the CLI structure
	into the buffer. If the buffer is too small the name is truncated,
	and a failure is returned. If the current process doesn't have
	a CLI structure, a 0 length string is put into the buffer and a
	failure is returned.

    INPUTS
	buf - Buffer for the name.
	len - Size of the buffer in bytes.

    RESULT
	!=0 on success, 0 on failure. IoErr() gives additional information
	in that case.

    NOTES
	Documented as returning ERROR_OBJECT_WRONG_TYPE if CLI structure
	is not present but actually it fallbacks to NameFromLock().

    EXAMPLE

    BUGS

    SEE ALSO
	SetCurrentDirName()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli;
    STRPTR cname;
    ULONG clen;
    BOOL ret = DOSTRUE;

    ASSERT_VALID_PROCESS(me);

    cli = BADDR(me->pr_CLI);
    if (cli == NULL)
    	return NameFromLock(me->pr_CurrentDir, buf, len);

    cname = AROS_BSTR_ADDR(cli->cli_SetName);
    clen = (ULONG)AROS_BSTR_strlen(cli->cli_SetName);
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
} /* GetCurrentDirName */

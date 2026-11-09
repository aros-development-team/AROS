/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Sets the name of the current program.
*/
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <string.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */

#include <proto/dos.h>

        AROS_LH1(BOOL, SetProgramName,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 95, Dos)

/*  FUNCTION
        Sets the name for the current program in the CLI structure. If the
        name doesn't fit the old name is kept and a failure is returned.
        If the current process doesn't have a CLI structure this function
        does nothing.

    INPUTS
        name - Name for the current program.

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

    return internal_SetProgramName(Cli(), name, DOSBase) ? DOSTRUE : DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* SetProgramName */


BOOL internal_SetProgramName(struct CommandLineInterface *cli,
    CONST_STRPTR name, struct DosLibrary *DOSBase)
{
    if (cli == NULL)
    {
        return FALSE;
    }

    return internal_SetCliString(cli->cli_CommandName, name);
}

/* Store the C string 'name' in the CLI BSTR field 'dest' (up to 255 chars) */
BOOL internal_SetCliString(BSTR dest, CONST_STRPTR name)
{
    CONST_STRPTR  s;
    STRPTR  bs;
    ULONG   namelen;

    s = name;

    while(*s++)
        ;

    namelen = s - name - 1;

    if (namelen > 255)
    {
        return FALSE;
    }

    bs = AROS_BSTR_ADDR(dest);

    AROS_BSTR_setstrlen(dest, namelen);
    CopyMem((APTR)name, bs, namelen);

    return TRUE;
}

/* Copy the CLI BSTR field 'src' to 'buf' as a C string. If it does not fit
 * in 'len' bytes it is cut, IoErr() is set to ERROR_LINE_TOO_LONG and
 * DOSFALSE returned. */
BOOL internal_GetCliString(struct Process *me, BSTR src, STRPTR buf, LONG len)
{
    STRPTR cname;
    ULONG clen;
    BOOL ret = DOSTRUE;

    cname = AROS_BSTR_ADDR(src);
    clen = (ULONG)AROS_BSTR_strlen(src);
    if (clen >= (len-1))
    {
        clen = len-1;
        me->pr_Result2 = ERROR_LINE_TOO_LONG;
        ret = DOSFALSE;
    }
    CopyMem(cname, buf, clen);
    buf[clen] = '\0';

    return ret;
}

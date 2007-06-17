/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: /aros/dos/src/rom/dos/open.c 26802 2007-06-11T21:17:31.715385Z rob  $

    Desc: Creates a pair of filehandles connected to each other
    Lang: english
*/
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, Pipe,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,       D1),
        AROS_LHA(BPTR,         *reader,    D2),
        AROS_LHA(BPTR,         *writer,    D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 160, Dos)

/*  FUNCTION
	Creates a pair of file handles connected to each other

    INPUTS
        name       - NULL-terminated name of the file
        reader     - Pointer to BPTR to store read handle in
        writer     - Pointer to BPTR to store write handle in

    RESULT
        DOSTRUE on success, DOSFALSE on failure. IoErr() gives additional
        information if the call failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *rfh, *wfh;
    struct IOFileSys iofs;
    LONG err;

    if ((rfh = (struct FileHandle *) AllocDosObject(DOS_FILEHANDLE, NULL)) == NULL) {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }
    if ((wfh = (struct FileHandle *) AllocDosObject(DOS_FILEHANDLE, NULL)) == NULL) {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }

    InitIOFS(&iofs, FSA_PIPE, DOSBase);
    err = DoIOFS(&iofs, NULL, name, DOSBase);

    if (err != 0) {
        FreeDosObject(rfh, DOS_FILEHANDLE);
        FreeDosObject(rfh, DOS_FILEHANDLE);
        return DOSFALSE;
    }

    rfh->fh_Device = iofs.IOFS.io_Device;
    rfh->fh_Unit   = iofs.IOFS.io_Unit;

    wfh->fh_Device = iofs.IOFS.io_Device;
    wfh->fh_Unit   = iofs.io_Union.io_PIPE.io_Writer;

    if (IsInteractive(MKBADDR(rfh)))
        SetVBuf(MKBADDR(rfh), NULL, BUF_LINE, -1);

    if (IsInteractive(MKBADDR(wfh)))
        SetVBuf(MKBADDR(wfh), NULL, BUF_LINE, -1);

    *reader = MKBADDR(rfh);
    *writer = MKBADDR(wfh);

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* Pipe */

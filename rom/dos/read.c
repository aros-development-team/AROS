/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read a couple of bytes from a file.
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(LONG, Read,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file,   D1),
        AROS_LHA(APTR, buffer, D2),
        AROS_LHA(LONG, length, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 7, Dos)

/*  FUNCTION
        Read some data from a given file. The request is directly
        given to the filesystem - no buffering is involved. For
        small amounts of data it's probably better to use the
        buffered I/O routines.

    INPUTS
        file   - filehandle
        buffer - pointer to buffer for the data
        length - number of bytes to read. The filesystem is
                 advised to try to fulfill the request as well
                 as possible.

    RESULT
        The number of bytes actually read, 0 if the end of the
        file was reached, -1 if an error happened. IoErr() will
        give additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to filehandle */
    struct FileHandle *fh = BADDR(file);
    LONG ret = -1;

    ASSERT_VALID_PTR(fh);
    ASSERT_VALID_PTR(buffer);

    D(bug("[Read] %x %x %d\n", fh, buffer, length));
    if (fh == NULL)
        SetIoErr(ERROR_INVALID_LOCK);
    else if (fh->fh_Type == BNULL)
        ret = 0;
    else
        ret = dopacket3(DOSBase, NULL, fh->fh_Type, ACTION_READ, fh->fh_Arg1, (SIPTR)buffer, length);
    D(bug("[Read]=%d\n", ret));

    return ret;

    AROS_LIBFUNC_EXIT
} /* Read */

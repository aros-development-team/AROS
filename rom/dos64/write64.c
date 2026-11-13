/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Write up to 64 bits worth of bytes to a file.
*/

#include <proto/exec.h>
#include "dos64_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos64.h>

        AROS_LH2QUAD1(QUAD, Write64,

/*  SYNOPSIS */
        AROS_LHA(BPTR,       file,   D1),
        AROS_LHA(CONST_APTR, buffer, D2),
        AROS_LHA2(QUAD, length, D3, D4),

/*  LOCATION */
        struct Dos64Base *, DOS64Base, 12, Dos64)

/*  FUNCTION
        Write some data to a given file with a 64-bit length. The
        request is passed to the filesystem in chunks the 32-bit
        ACTION_WRITE packet can carry, so it works with every
        filesystem - no buffering is involved.

    INPUTS
        file   - filehandle
        buffer - pointer to the data to write
        length - number of bytes to write.

    RESULT
        The number of bytes actually written, -1 if an error happened.
        If an error occurs after some data has already been
        transferred, the number of bytes written so far is returned
        and IoErr() gives the error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/Write(), Read64(), Seek64()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct FileHandle *fh = BADDR(file);
    QUAD done = 0;

    if (fh == NULL)
    {
        SetIoErr(ERROR_INVALID_LOCK);
        return -1;
    }

    if (length < 0)
    {
        SetIoErr(ERROR_BAD_NUMBER);
        return -1;
    }

    if (fh->fh_Type == BNULL)
        return length;

    while (done < length)
    {
        LONG chunk = (length - done > DOS64_IOCHUNK)
            ? DOS64_IOCHUNK : (LONG)(length - done);
        SIPTR ret;

        ret = dos64_SendPkt(DOS64Base, fh->fh_Type, ACTION_WRITE,
                            fh->fh_Arg1, (SIPTR)((CONST UBYTE *)buffer + done),
                            chunk, 0, 0, NULL);
        if (ret < 0)
            return done ? done : -1;
        done += ret;
        if (ret < chunk)
            break;
    }

    return done;

    AROS_LIBFUNC_EXIT
} /* Write64 */

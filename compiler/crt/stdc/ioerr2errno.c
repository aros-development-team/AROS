/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: errno internals
*/

#include <dos/dos.h>
#include <dos/dosasl.h>
#include <errno.h>

/****************************************************************************
    NAME
        int ioerr2errno(

    SYNOPSIS
        int ioerr)

    FUNCTION
        Convert a DOS error number to a stdc error number.

    INPUTS
        ioerr - Zero or a positive DOS error number no greater than
                INT_MAX - MAX_ERRNO.

    RESULT
        Zero for input zero. The following DOS errors map to errno
        constants:

        ERROR_OBJECT_WRONG_TYPE        EINVAL
        ERROR_INVALID_COMPONENT_NAME   EINVAL
        ERROR_NO_FREE_STORE            ENOMEM
        ERROR_DEVICE_NOT_MOUNTED       ENOENT
        ERROR_OBJECT_NOT_FOUND         ENOENT
        ERROR_OBJECT_EXISTS            EEXIST
        ERROR_BUFFER_OVERFLOW          ENOBUFS
        ERROR_BREAK                    EINTR
        ERROR_FILE_NOT_OBJECT          ENOEXEC
        ERROR_NOT_EXECUTABLE           ENOEXEC
        ERROR_OBJECT_IN_USE            EBUSY
        ERROR_DIR_NOT_FOUND            ENOTDIR
        ERROR_RENAME_ACROSS_DEVICES    EXDEV

        Other values return MAX_ERRNO + ioerr.

    NOTES
        Include <errno.h>. The function neither reads IoErr() nor
        changes errno or the DOS error state. Assign the result to
        errno when required.

        These mappings describe stdc.library. The selected C link
        library may override ioerr2errno(); __stdc_ioerr2errno()
        selects the stdc.library implementation.

        The stdc strerror() implementation recognizes positive DOS
        errors encoded with the MAX_ERRNO offset.

    SEE ALSO
        strerror(), dos.library/IoErr()

*****************************************************************************/
int ioerr2errno (int ioerr)
{
    switch (ioerr)
    {
        case 0:
            return 0;

        case ERROR_OBJECT_WRONG_TYPE:
        case ERROR_INVALID_COMPONENT_NAME:
            return EINVAL;

        case ERROR_NO_FREE_STORE:
            return ENOMEM;

        case ERROR_DEVICE_NOT_MOUNTED:
        case ERROR_OBJECT_NOT_FOUND:
            return ENOENT;

        case ERROR_OBJECT_EXISTS:
            return EEXIST;

        case ERROR_BUFFER_OVERFLOW:
            return ENOBUFS;

        case ERROR_BREAK:
            return EINTR;

        case ERROR_FILE_NOT_OBJECT:
        case ERROR_NOT_EXECUTABLE:
            return ENOEXEC;

        case ERROR_OBJECT_IN_USE:
            return EBUSY;

        case ERROR_DIR_NOT_FOUND:
            return ENOTDIR;

        case ERROR_RENAME_ACROSS_DEVICES:
            return EXDEV;
    }

    return MAX_ERRNO+ioerr;
} /* ioerr2errno */

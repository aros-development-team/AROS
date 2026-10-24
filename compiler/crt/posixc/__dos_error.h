/*
    Copyright (C) 2026 John Knipper.
    Distributed under the AROS Public License, version 1.1.

    Translate an immediately captured native failure into the caller's POSIX
    error namespace. The caller still owns its per-task errno assignment.
*/
#ifndef __POSIXC_DOS_ERROR_H
#define __POSIXC_DOS_ERROR_H

#include <dos/dos.h>
#include <errno.h>

static int __posixc_dos_error(LONG error)
{
    switch (error)
    {
        case 0:
            return EIO; /* a failed operation must not report "No error" */
        case ERROR_ACTION_NOT_KNOWN:
        case ERROR_NOT_IMPLEMENTED:
            return ENOSYS;
        default:
            return __stdc_ioerr2errno(error);
    }
}

#endif

/*
 * Original Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Based upon usergroup.library from AmiTCP/IP.
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#include <aros/libcall.h>
#include <proto/exec.h>
#include "base.h"

#include <assert.h>

#include <sys/errno.h>
#include <exec/errors.h>

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#include <proto/utility.h>

#include "assert.h"

/*
   * SetErrno - Errno handling
 */
void ug_SetErrno(struct Library *ugBase, int _en)
{
    struct UserGroupBase *UserGroupBase = (struct UserGroupBase *)ugBase;
    UserGroupBase->errnolast = _en;
    if (UserGroupBase->errnop)
        switch (UserGroupBase->errnosize) {
        case es_byte:
            *(UBYTE *)UserGroupBase->errnop = _en;
            break;
        case es_word:
            *(UWORD *)UserGroupBase->errnop = _en;
            break;
        case es_long:
            *(ULONG *)UserGroupBase->errnop = _en;
            break;
        }
}

/****** usergroup.library/ug_GetErr ****************************************

    NAME
        ug_GetErr - get current error code

    SYNOPSIS
        error = ug_GetErr(void)
         D0

        int ug_GetErr(void)

    FUNCTION
        Most usergroup.library functions return -1 to indicate an error.
        When this happens (or whatever the defined error return for the
        routine) this routine may be called to determine more information.
        The default startup function will redirect the error codes also into
        the global variable `errno'.

        Note: there is no guarantee as to the value returned from ug_GetErr()
        after a successful operation.

    RESULTS
        error - error code

    SEE ALSO
        ug_StrError(), ug_SetupContextTags(), dos.library/IoErr()

****************************************************************************
*/

/*
 * Get errno
 */
AROS_LH0 (int, ug_GetErr,
          struct UserGroupBase *, UserGroupBase, 6, Usergroup)
{
    AROS_LIBFUNC_INIT

    return UserGroupBase->errnolast;

    AROS_LIBFUNC_EXIT
}

#if !defined(__AROS__)
static const BYTE ioerr2errno[-IOERR_SELFTEST] = {
    /* IOERR_OPENFAIL */
    ENOENT,
    /* IOERR_ABORTED */
    EINTR,
    /* IOERR_NOCMD */
    ENODEV,
    /* IOERR_BADLENGTH */
    EBUSY,
    /* IOERR_BADADDRESS */
    EFAULT,
    /* IOERR_UNITBUSY */
    EBUSY,
    /* IOERR_SELFTEST */
    ENXIO,
};
#endif

void SetDeviceErr(struct Library *ugBase)
{
    struct UserGroupBase *UserGroupBase = (struct UserGroupBase *)ugBase;
    short err;

    if (UserGroupBase->nireq)
        err = UserGroupBase->nireq->io_Error;
    else
        err = ENOMEM;

    if (err < 0) {
        if (err >= IOERR_SELFTEST) {
#if !defined(__AROS__)
            err = ioerr2errno[-1 - err];
#else
            err = ioerr2errno(-1 - err);
#endif
        } else
            err = EIO;
    }

    ug_SetErrno(ugBase, err);
}

/*
 * Get error string
 */
#define EUNKNOWN "Unknown error"

static const char * const __ug_errlist[] = {
    "Undefined error: 0",			  /*  0 - ENOERROR */
    "Operation not permitted",		  /*  1 - EPERM */
    "No such entry",			  /*  2 - ENOENT */
    "No such process",		          /*  3 - ESRCH */
    "Interrupted library call",		  /*  4 - EINTR */
    "Input/output error",			  /*  5 - EIO */
    "Device not configured",		  /*  6 - ENXIO */
    "Argument list too long",		  /*  7 - E2BIG */
    "Exec format error",			  /*  8 - ENOEXEC */
    "Bad file descriptor",		  /*  9 - EBADF */
    "No child processes",			  /* 10 - ECHILD */
    "Resource deadlock avoided",	          /* 11 - EDEADLK */
    "Cannot allocate memory",		  /* 12 - ENOMEM */
    "Permission denied",			  /* 13 - EACCES */
    "Bad address",			  /* 14 - EFAULT */
    "Block device required",		  /* 15 - ENOTBLK */
    "Device busy",			  /* 16 - EBUSY */
    "File exists",			  /* 17 - EEXIST */
    "Cross-device link",			  /* 18 - EXDEV */
    "Operation not supported by device",	  /* 19 - ENODEV */
    "Not a directory",			  /* 20 - ENOTDIR */
    "Is a directory",			  /* 21 - EISDIR */
    "Invalid argument",			  /* 22 - EINVAL */
    "Too many open files in system",	  /* 23 - ENFILE */
    "Too many open files",		  /* 24 - EMFILE */
    "Inappropriate operation for device",   /* 25 - ENOTTY */
    "Text file busy",			  /* 26 - ETXTBSY */
    "File too large",			  /* 27 - EFBIG */
    "No space left on device",		  /* 28 - ENOSPC */
    "Illegal seek",			  /* 29 - ESPIPE */
    "Read-only file system",		  /* 30 - EROFS */
    "Too many links",			  /* 31 - EMLINK */
    "Broken pipe",			  /* 32 - EPIPE */

    /* math software */
    "Numerical argument out of domain",	  /* 33 - EDOM */
    "Result too large",			  /* 34 - ERANGE */
    /* non-blocking and interrupt i/o */
    "Resource temporarily unavailable",	  /* 35 - EAGAIN */
    /* 35 - EWOULDBLOCK */
    "Operation now in progress",		  /* 36 - EINPROGRESS */
    "Operation already in progress",	  /* 37 - EALREADY */

    /* ipc/network software -- argument errors */
    "Socket operation on non-socket",	  /* 38 - ENOTSOCK */
    "Destination address required",	  /* 39 - EDESTADDRREQ */
    "Message too long",			  /* 40 - EMSGSIZE */
    "Protocol wrong type for socket",	  /* 41 - EPROTOTYPE */
    "Protocol not available",		  /* 42 - ENOPROTOOPT */
    "Protocol not supported",		  /* 43 - EPROTONOSUPPORT */
    "Socket type not supported",		  /* 44 - ESOCKTNOSUPPORT */
    "Operation not supported",		  /* 45 - EOPNOTSUPP */
    "Protocol family not supported",	  /* 46 - EPFNOSUPPORT */
    /* 47 - EAFNOSUPPORT */
    "Address family not supported by protocol family",
    "Address already in use",		  /* 48 - EADDRINUSE */
    "Can't assign requested address",	  /* 49 - EADDRNOTAVAIL */

    /* ipc/network software -- operational errors */
    "Network is down",			  /* 50 - ENETDOWN */
    "Network is unreachable",		  /* 51 - ENETUNREACH */
    "Network dropped connection on reset",  /* 52 - ENETRESET */
    "Software caused connection abort",	  /* 53 - ECONNABORTED */
    "Connection reset by peer",		  /* 54 - ECONNRESET */
    "No buffer space available",		  /* 55 - ENOBUFS */
    "Socket is already connected",	  /* 56 - EISCONN */
    "Socket is not connected",		  /* 57 - ENOTCONN */
    "Can't send after socket shutdown",	  /* 58 - ESHUTDOWN */
    "Too many references: can't splice",	  /* 59 - ETOOMANYREFS */
    "Connection timed out",		  /* 60 - ETIMEDOUT */
    "Connection refused",			  /* 61 - ECONNREFUSED */

    "Too many levels of symbolic links",	  /* 62 - ELOOP */
    "File name too long",			  /* 63 - ENAMETOOLONG */

    /* should be rearranged */
    "Host is down",			  /* 64 - EHOSTDOWN */
    "No route to host",			  /* 65 - EHOSTUNREACH */
    "Directory not empty",		  /* 66 - ENOTEMPTY */
};

static const int __ug_nerr = ENOTEMPTY + 1;

/****** usergroup.library/ug_StrError **************************************

    NAME
        ug_StrError - Return the text associated with error code

    SYNOPSIS
        text = ug_StrError(code)
         D0                   D1

        const char *ug_StrError(LONG);

    FUNCTION
        The strerror() function maps the error number specified by the
        errnum parameter to a language-dependent error message string, and
        returns a pointer to the string.  The string pointed to by the
        return value should not be modified by the program, but may be
        overwritten by a subsequent call to this function.

    INPUTS
        code - error code returned by ug_GetErr() function.

    RESULT
        text - text associated with the error code.

    NOTES
        The current implementation will understands also the negative IO
        error codes.

    BUGS
        Currently only language available is English.

    SEE ALSO
        ug_GetErr()

******************************************************************************
*/

AROS_LH1I (const char *, ug_StrError,
           AROS_LHA(LONG, code, D1),
           struct Library *, UserGroupBase, 7, Usergroup)
{
    AROS_LIBFUNC_INIT

    if (code < 0) {
        if (code >= IOERR_SELFTEST) {
#if !defined(__AROS__)
            code = ioerr2errno[-1 - code];
#else
            code = ioerr2errno(-1 - code);
#endif
        } else
            code = EIO;
    }

    if (code >= __ug_nerr)
        return EUNKNOWN;
    else
        return __ug_errlist[code];

    AROS_LIBFUNC_EXIT
}

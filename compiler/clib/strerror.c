/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strerror().
*/

#include "__arosc_privdata.h"

#include <proto/dos.h>
#include <clib/macros.h>
#include <errno.h>
#include <stdio.h>

static const char * _errstrings[];

/*****************************************************************************

    NAME */
#include <string.h>

	char * strerror (

/*  SYNOPSIS */
	int n)

/*  FUNCTION
	Returns a readable string for an error number in errno.

    INPUTS
	n - The contents of errno or a #define from errno.h

    RESULT
	A string describing the error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    if (n > MAX_ERRNO)
    {
        struct aroscbase *aroscbase = __aros_getbase_aroscbase();

	Fault(n - MAX_ERRNO, NULL, aroscbase->acb_fault_buf, sizeof(aroscbase->acb_fault_buf));

	return aroscbase->acb_fault_buf;
    }
    else
    {
        char *s;

        s = (char *)_errstrings[MIN(n, ELAST+1)];

        if (s == NULL)
            s = (char *)"Errno out of range";

        return s;
    }
} /* strerror */


static const char * _errstrings[ELAST + 2] =
{
    /* 0	       */	"No error",
    /* EPERM	       */	"Operation not permitted",
    /* ENOENT	       */	"No such file or directory",
    /* ESRCH	       */	"No such process",
    /* EINTR	       */	"Interrupted system call",
    /* EIO	       */	"I/O error",
    /* ENXIO	       */	"No such device or address",
    /* E2BIG	       */	"Arg list too long",
    /* ENOEXEC	       */	"Exec format error",
    /* EBADF	       */	"Bad file number",
    /* ECHILD	       */	"No child processes",
    /* EDEADLK	       */	"Resource deadlock would occur",
    /* ENOMEM	       */	"Out of memory",
    /* EACCES	       */	"Permission denied",
    /* EFAULT	       */	"Bad address",
    /* NA	       */	NULL,
    /* EBUSY	       */	"Device or resource busy",
    /* EEXIST	       */	"File exists",
    /* EXDEV	       */	"Cross-device link",
    /* ENODEV	       */	"No such device",
    /* ENOTDIR	       */	"Not a directory",
    /* EISDIR	       */	"Is a directory",
    /* EINVAL	       */	"Invalid argument",
    /* ENFILE	       */	"File table overflow",
    /* EMFILE	       */	"Too many open files",
    /* ENOTTY	       */	"Not a typewriter",
    /* ETXTBSY	       */	"Text file busy",
    /* EFBIG	       */	"File too large",
    /* ENOSPC	       */	"No space left on device",
    /* ESPIPE	       */	"Illegal seek",
    /* EROFS	       */	"Read-only file system",
    /* EMLINK	       */	"Too many links",
    /* EPIPE	       */	"Broken pipe",
    /* NA	       */	NULL,
    /* ERANGE	       */	"Math result not representable",
    /* EAGAIN	       */	"Try again",
    /* EINPROGRESS     */	"Operation now in progress",
    /* EALREADY        */	"Operation already in progress",
    /* ENOTSOCK        */	"Socket operation on non-socket",
    /* EDESTADDRREQ    */	"Destination address required",
    /* EMSGSIZE        */	"Message too long",
    /* EPROTOTYPE      */	"Protocol wrong type for socket",
    /* ENOPROTOOPT     */	"Protocol not available",
    /* EPROTONOSUPPORT */	"Protocol not supported",
    /* ESOCKTNOSUPPORT */	"Socket type not supported",
    /* EOPNOTSUPP      */	"Operation not supported on transport endpoint",
    /* EPFNOSUPPORT    */	"Protocol family not supported",
    /* EAFNOSUPPORT    */	"Address family not supported by protocol",
    /* EADDRINUSE      */	"Address already in use",
    /* EADDRNOTAVAIL   */	"Cannot assign requested address",
    /* ENETDOWN        */	"Network is down",
    /* ENETUNREACH     */	"Network is unreachable",
    /* ENETRESET       */	"Network dropped connection because of reset",
    /* ECONNABORTED    */	"Software caused connection abort",
    /* ECONNRESET      */	"Connection reset by peer",
    /* ENOBUFS	       */	"No buffer space available",
    /* EISCONN	       */	"Transport endpoint is already connected",
    /* ENOTCONN        */	"Transport endpoint is not connected",
    /* ESHUTDOWN       */	"Cannot send after transport endpoint shutdown",
    /* NA	       */	NULL,
    /* ETIMEDOUT       */	"Connection timed out",
    /* ECONNREFUSED    */	"Connection refused",
    /* ELOOP	       */	"Too many symbolic links encountered",
    /* ENAMETOOLONG    */	"File name too long",
    /* EHOSTDOWN       */	"Host is down",
    /* EHOSTUNREACH    */	"No route to host",
    /* ENOTEMPTY       */	"Directory not empty",
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* EDQUOT	       */	"Quota exceeded",
    /* ESTALE	       */	"Stale NFS file handle",
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* ENOLCK	       */	"No record locks available",
    /* ENOSYS	       */	"Function not implemented",
    /* NA              */	NULL,
    /* NA	       */	NULL,
    /* NA	       */	NULL,
    /* EIDRM	       */	"Identifier removed",
    /* ENOMSG	       */	"No message of desired type",
    /* EOVERFLOW       */	"Value too large for defined data type",
    /* EILSEQ	       */	"Illegal byte sequence",
    /* ENOTSUP	       */	"Not supported",
    /* ECANCELED       */	"Operation canceled",
    /* EBADMSG	       */	"Bad or Corrupt message",
    /* ENODATA	       */	"No message available",
    /* ENOSR	       */	"No STREAM resources",
    /* ENOSTR	       */	"Not a STREAM",
    /* ETIME	       */	"STREAM ioctl timeout",
    /* NA	       */	NULL,
    /* EMULTIHOP       */	"Multihop attempted",
    /* ENOLINK	       */	"Link has been severed",
    /* EPROTO	       */	"Protocol error",
    /* Too high        */	NULL
};

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strerror().
*/

#include <clib/macros.h>
#include <errno.h>

static const char * _errstrings[];

/*****************************************************************************

    NAME */
#include <string.h>

	char * __posixc_strerror (

/*  SYNOPSIS */
	int n)

/*  FUNCTION
	Returns a readable string for an error number in errno.

    INPUTS
	n - The contents of errno or a #define from errno.h

    RESULT
	A string describing the error.

    NOTES
        This function is used to override the strerror() function of
        stdc.library to handle the extra errnos from posixc.library.
        It is aliased as strerror() in libposixc.a

    EXAMPLE

    BUGS

    SEE ALSO
        stdc.library/__stdc_strerror(), stdc.library/strerror()

    INTERNALS

******************************************************************************/
{
    char *s;
    
    s = (char *)_errstrings[MIN(n, __POSIXC_ELAST+1)];

    return (s != NULL ? s : __stdc_strerror(n));
} /* strerror */


/* Only POSIX.1-2008 specific codes are here
   C99 codes will be handled by calling __stdc_strerror()
 */
static const char * _errstrings[__POSIXC_ELAST+2] =
{
    /* 0	       */	"No error",
    /* EPERM	       */	"Operation not permitted",
    /* C99   	       */	NULL,
    /* ESRCH	       */	"No such process",
    /* C99  	       */	NULL,
    /* EIO	       */	"I/O error",
    /* ENXIO	       */	"No such device or address",
    /* E2BIG	       */	"Arg list too long",
    /* C99    	       */	NULL,
    /* EBADF	       */	"Bad file number",
    /* ECHILD	       */	"No child processes",
    /* EDEADLK	       */	"Resource deadlock would occur",
    /* C99   	       */	NULL,
    /* C99   	       */	NULL,
    /* EFAULT	       */	"Bad address",
    /* NA	       */	NULL,
    /* C99  	       */	NULL,
    /* C99   	       */	NULL,
    /* C99  	       */	NULL,
    /* ENODEV	       */	"No such device",
    /* C99    	       */	NULL,
    /* EISDIR	       */	"Is a directory",
    /* C99   	       */	NULL,
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
    /* C99	       */	NULL,
    /* C99    	       */	NULL,
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
    /* C99    	       */	NULL,
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
    /* C99   	       */	NULL,
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

#ifndef _POSIXC_ERRNO_H_
#define _POSIXC_ERRNO_H_

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 header file errno.h

    On AROS we take the NetBSD errno numbering as reference for backwards
    compatibility with the errno numbering used in the bsdsocket.library.
*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)errno.h	8.5 (Berkeley) 1/21/94
 */

/* C99 */
#include <aros/stdc/errno.h>

/* Other codes defined by POSIX.1-2008 */
#define	E2BIG		7		/* Argument list too long */
#define	EADDRINUSE	48		/* Address already in use */
#define	EADDRNOTAVAIL	49		/* Can't assign requested address */
#define	EAFNOSUPPORT	47		/* Address family not supported by protocol family */
#define	EAGAIN		35		/* Resource temporarily unavailable */
#define	EALREADY	37		/* Operation already in progress */
#define	EBADF		9		/* Bad file descriptor */
#define EBADMSG		88		/* Bad or Corrupt message */
#define	ECANCELED	87		/* Operation canceled */
#define	ECHILD		10		/* No child processes */
#define	ECONNABORTED	53		/* Software caused connection abort */
#define	ECONNREFUSED	61		/* Connection refused */
#define	ECONNRESET	54		/* Connection reset by peer */
#define	EDEADLK		11		/* Resource deadlock avoided */
#define	EDESTADDRREQ	39		/* Destination address required */
#define	EDQUOT		69		/* Disc quota exceeded */
#define	EFAULT		14		/* Bad address */
#define	EFBIG		27		/* File too large */
#define	EHOSTUNREACH	65		/* No route to host */
#define	EIDRM		82		/* Identifier removed */
#define	EINPROGRESS	36		/* Operation now in progress */
#define	EIO		5		/* Input/output error */
#define	EISCONN		56		/* Socket is already connected */
#define	EISDIR		21		/* Is a directory */
#define	ELOOP		62		/* Too many levels of symbolic links */
#define	EMFILE		24		/* Too many open files */
#define	EMLINK		31		/* Too many links */
#define	EMSGSIZE	40		/* Message too long */
#define EMULTIHOP	94		/* Multihop attempted */
#define	ENAMETOOLONG	63		/* File name too long */
#define	ENETDOWN	50		/* Network is down */
#define	ENETRESET	52		/* Network dropped connection on reset */
#define	ENETUNREACH	51		/* Network is unreachable */
#define	ENFILE		23		/* Too many open files in system */
#define ENODATA		89		/* No message available */
#define	ENODEV		19		/* Operation not supported by device */
#define	ENOLCK		77		/* No locks available */
#define ENOLINK		95		/* Link has been severed */
#define	ENOMSG		83		/* No message of desired type */
#define	ENOPROTOOPT	42		/* Protocol not available */
#define	ENOSPC		28		/* No space left on device */
#define ENOSR		90		/* No STREAM resources */
#define ENOSTR		91		/* Not a STREAM */
#define	ENOSYS		78		/* Function not implemented */
#define	ENOTCONN	57		/* Socket is not connected */
#define	ENOTEMPTY	66		/* Directory not empty */
/* NOTIMPL ENORECOVERABLE */
#define	ENOTSOCK	38		/* Socket operation on non-socket */
#define ENOTSUP		86		/* Not supported */
#define	ENOTTY		25		/* Inappropriate ioctl for device */
#define	ENXIO		6		/* Device not configured */
#define	EOPNOTSUPP	45		/* Operation not supported on socket */
#define	EOVERFLOW	84		/* Value too large to be stored in data type */
/* NOTIMPL EOWNERDEAD */
#define	EPERM		1		/* Operation not permitted */
#define	EPIPE		32		/* Broken pipe */
#define EPROTO		96		/* Protocol error */
#define	EPROTONOSUPPORT	43		/* Protocol not supported */
#define	EPROTOTYPE	41		/* Protocol wrong type for socket */
#define	EROFS		30		/* Read-only file system */
#define	ESPIPE		29		/* Illegal seek */
#define	ESRCH		3		/* No such process */
#define	ESTALE		70		/* Stale NFS file handle */
#define ETIME		92		/* STREAM ioctl timeout */
#define	ETIMEDOUT	60		/* Connection timed out */
#define	ETXTBSY		26		/* Text file busy */
#define	EWOULDBLOCK	EAGAIN		/* Operation would block */


/* Some compatibility defines mainly for AROSTCP */
#ifdef __BSD_VISIBLE
#define	EHOSTDOWN	64		/* Host is down */
#define	EPFNOSUPPORT	46		/* Protocol family not supported */
#define	ERESTART	-1		/* restart syscall */
#define	ESOCKTNOSUPPORT	44		/* Socket type not supported */
#define	ESHUTDOWN	58		/* Can't send after socket shutdown */
#endif

#define __POSIXC_ELAST	EPROTO		/* Points to highest used errno in this include */

#endif /* _POSIXC_ERRNO_H_ */

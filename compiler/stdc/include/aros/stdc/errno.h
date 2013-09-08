#ifndef _STDC_ERRNO_H_
#define _STDC_ERRNO_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 header file errno.h

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
#include <aros/system.h>
#include <sys/arosc.h>


/* C99 required error codes */
#define	EDOM		33		/* Numerical argument out of domain */
#define	EILSEQ		85		/* Illegal byte sequence */
#define	EINVAL		22		/* Invalid argument */
#define	ERANGE		34		/* Result too large */


/* Codes used for error conversion of DOS error codes */
#define	EACCES		13		/* Permission denied */
#define	EBUSY		16		/* Device busy */
#define	EEXIST		17		/* File exists */
#define	EINTR		4		/* Interrupted system call */
#define	ENOBUFS		55		/* No buffer space available */
#define	ENOENT		2		/* No such file or directory */
#define	ENOEXEC		8		/* Exec format error */
#define	ENOMEM		12		/* Cannot allocate memory */
#define	ENOTDIR		20		/* Not a directory */
#define	EXDEV		18		/* Cross-device link */

/* MAX_ERRNO is currently used by ioerr2errno conversion
   To keep backwards compatibility it's value should not change
 */
#define MAX_ERRNO	1000  		/* Numbers should never be bigger than this value */


__BEGIN_DECLS

__pure static __inline__ int *__get_errno_ptr(void);
__pure static __inline__ int *__get_errno_ptr(void)
{
    return &__get_arosc_userdata()->acud_errno;
}
#define errno (*__get_errno_ptr())

/* AROS specific functions to translate DOS error numbers to errno.
   ioerrno2errno() will always call the function for the selected C
   linklib, __arosc_ioerr2errno() is always the arosc.library version.
 */
int ioerr2errno(int ioerr);
int __arosc_ioerr2errno(int ioerr); 

__END_DECLS

#endif /* _STDC_ERRNO_H_ */

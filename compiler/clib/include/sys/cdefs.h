#ifndef _SYS_CDEFS_H_
#define _SYS_CDEFS_H_
/*
 * Copyright 1995-2002, The AROS Development Team. All rights reserved.
 * $Id$
 *
 * <sys/cdefs.h> header file, would you believe it's mostly the same as
 * <aros/system.h>, except that here we define all the things that
 * BSD/UNIX/POSIX like systems are expecting.
 *
 * The namespace checks below are
 *
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *      @(#)cdefs.h     8.8 (Berkeley) 1/9/95
 * $FreeBSD: src/sys/sys/cdefs.h,v 1.75 2003/07/25 18:40:36 gad Exp $
 */

#include <aros/system.h>

#if defined(__STDC__) || defined(__cplusplus)
#define __P(protos)	protos
#else
#define __P(protos)	()
#endif

#if !defined(__IDSTRING)
#   if defined(__GNUC__) && defined(__ELF__)
#       define __IDSTRING(name,str)         __asm__(".ident\t\"" str "\"")
#   else
#       define __IDSTRING(name,str)         static const char name[] __unused = str
#   endif
#endif

/* Need to protect __RCSID against the host system headers */
#if !defined(__RCSID)
#   define __RCSID(id)  __IDSTRING(rcsid,id)
#endif

/* FreeBSD's, it's basically yet another IDSTRING like thing. */
#define __FBSDID(id)     __IDSTRING(__CONCAT(__rcsid_,__LINE__),id)

/*****************************************************************************
    IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT etc

    If you do not understand what the following code does, then please do
    not change it. Basically various standards (POSIX, ISO C, etc) are
    very pedantic about what is visible and what is not.

    The following code from FreeBSD's cdefs.h basically does all the
    standards check. That is where the BSD licenced part comes from above.

    This should be compatible with *BSD, and hopefully Linux as well.
    Other forms of UNIX might have some more trouble.
*****************************************************************************/

/*-
 * POSIX.1 requires that the macros we test be defined before any standard
 * header file is included.
 *
 * Here's a quick run-down of the versions:
 *  defined(_POSIX_SOURCE)              1003.1-1988
 *  _POSIX_C_SOURCE == 1                1003.1-1990
 *  _POSIX_C_SOURCE == 2                1003.2-1992 C Language Binding Option
 *  _POSIX_C_SOURCE == 199309           1003.1b-1993
 *  _POSIX_C_SOURCE == 199506           1003.1c-1995, 1003.1i-1995,
 *                                      and the omnibus ISO/IEC 9945-1: 1996
 *  _POSIX_C_SOURCE == 200112           1003.1-2001
 *
 * In addition, the X/Open Portability Guide, which is now the Single UNIX
 * Specification, defines a feature-test macro which indicates the version of
 * that specification, and which subsumes _POSIX_C_SOURCE.
 *
 * Our macros begin with two underscores to avoid namespace screwage.
 */

/* Deal with IEEE Std. 1003.1-1990, in which _POSIX_C_SOURCE == 1. */
#if _POSIX_C_SOURCE == 1
#undef _POSIX_C_SOURCE          /* Probably illegal, but beyond caring now. */
#define _POSIX_C_SOURCE         199009
#endif

/* Deal with IEEE Std. 1003.2-1992, in which _POSIX_C_SOURCE == 2. */
#if _POSIX_C_SOURCE == 2
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE         199209
#endif

/* Deal with various X/Open Portability Guides and Single UNIX Spec. */
#ifdef _XOPEN_SOURCE
#if _XOPEN_SOURCE - 0 >= 600
#define __XSI_VISIBLE           600
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE         200112
#elif _XOPEN_SOURCE - 0 >= 500
#define __XSI_VISIBLE           500
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE         199506
#endif
#endif

/*
 * Deal with all versions of POSIX.  The ordering relative to the tests above is
 * important.
 */
#if defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE         198808
#endif
#ifdef _POSIX_C_SOURCE
#if _POSIX_C_SOURCE >= 200112
#define __POSIX_VISIBLE         200112
#define __ISO_C_VISIBLE         1999
#elif _POSIX_C_SOURCE >= 199506
#define __POSIX_VISIBLE         199506
#define __ISO_C_VISIBLE         1990
#elif _POSIX_C_SOURCE >= 199309
#define __POSIX_VISIBLE         199309
#define __ISO_C_VISIBLE         1990
#elif _POSIX_C_SOURCE >= 199209
#define __POSIX_VISIBLE         199209
#define __ISO_C_VISIBLE         1990
#elif _POSIX_C_SOURCE >= 199009
#define __POSIX_VISIBLE         199009
#define __ISO_C_VISIBLE         1990
#else
#define __POSIX_VISIBLE         198808
#define __ISO_C_VISIBLE         0
#endif /* _POSIX_C_SOURCE */
#else
/*-
 * Deal with _ANSI_SOURCE:
 * If it is defined, and no other compilation environment is explicitly
 * requested, then define our internal feature-test macros to zero.  This
 * makes no difference to the preprocessor (undefined symbols in preprocessing
 * expressions are defined to have value zero), but makes it more convenient for
 * a test program to print out the values.
 *
 * If a program mistakenly defines _ANSI_SOURCE and some other macro such as
 * _POSIX_C_SOURCE, we will assume that it wants the broader compilation
 * environment (and in fact we will never get here).
 */
#if defined(_ANSI_SOURCE)       /* Hide almost everything. */
#define __POSIX_VISIBLE         0
#define __XSI_VISIBLE           0
#define __BSD_VISIBLE           0
#define __ISO_C_VISIBLE         1990
#elif defined(_C99_SOURCE)      /* Localism to specify strict C99 env. */
#define __POSIX_VISIBLE         0
#define __XSI_VISIBLE           0
#define __BSD_VISIBLE           0
#define __ISO_C_VISIBLE         1999
#else                           /* Default environment: show everything. */
#define __POSIX_VISIBLE         200112
#define __XSI_VISIBLE           600
#define __BSD_VISIBLE           1
#define __ISO_C_VISIBLE         1999
#endif
#endif

/*****************************************************************************
    END OF IMPORTANT STUFF
 ****************************************************************************/

/*
    BSD systems like to see BYTE_ORDER and friends.

    FreeBSD 5 in its pedantic namespace also has versions with underscores
    to make it easier to port stuff, create them as well.
*/
#define _LITTLE_ENDIAN      1234
#define _BIG_ENDIAN         4321
#define _PDP_ENDIAN         3412

#if AROS_BIG_ENDIAN
#   define _BYTE_ORDER  _BIG_ENDIAN
#else
#   define _BYTE_ORDER  _LITTLE_ENDIAN
#endif

#if __BSD_VISIBLE
#   define LITTLE_ENDIAN    _LITTLE_ENDIAN
#   define BIG_ENDIAN       _BIG_ENDIAN
#   define PDP_ENDIAN       _PDP_ENDIAN
#   define BYTE_ORDER       _BYTE_ORDER
#endif

#endif /* _SYS_CDEFS_H_ */

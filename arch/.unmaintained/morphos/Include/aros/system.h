#pragma pack(2)
#ifndef AROS_SYSTEM_H
#define AROS_SYSTEM_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Analyse the current kind of system and compiler.
    Lang: english
*/

#ifndef __AROS__
#   define __AROS__
#endif
#ifndef AROS_MACHINE_H
#   include <aros/machine.h>
#endif

/**************************************
		Defines
**************************************/
/* 1. Analyze system: Specify a definitive define for each system */
#if defined(_AMIGA) || defined(AMIGA)
#   ifndef _AMIGA /* One define for each system */
#	define _AMIGA
#   endif
#endif

/*
 * 2. Analyze compiler for STD C/C++.
 *
 * We test for the following:
 * a.	extern "C" linkage required for programs.
 * b.	inline, const, volatile, restrict keywords defined in
 *	newer C/C++ standards.
 */

#if defined(__cplusplus)
#   define EXTERN extern "C"
#   define BEGIN_EXTERN     extern "C" {
#   define END_EXTERN	    };
#else
#   define EXTERN extern
#   define BEGIN_EXTERN
#   define END_EXTERN
#endif

#if defined(__STDC__) || defined(__cplusplus)
#define	    __const__	    const
#define	    __inline__	    inline
#define	    __volatile__    volatile

/*
 * C99 defines a new keyword restrict that can help do optimisation where
 * pointers are used in programs. We'd like to support optimisation :-)
 */
#if defined(__STDC__VERSION__) &&  __STDC__VERSION__ >= 199901L
#define	    __restrict__    restrict
#else
#define	    __restrict__
#define	    restrict
#endif

#else
#define	    __const__
#define	    const
#define	    __inline__
#define	    inline
#define	    __volatile__
#define	    volatile
#define	    __restrict__
#define	    restrict
#endif

/* 3. Macros for making things more efficient */
#if __GNUC__ < 2 || __GNUC__ == 2 && __GNUC_MINOR__ < 5
#define __unused
#define __noreturn
#define __noeffect
#endif
#if __GNUC__ == 2 && __GNUC_MINOR__ >= 5 && __GNUC_MINOR__ < 7
#define __unused
#define __noreturn  __attribute__((__noreturn__))
#define __noeffect  __attribute__((__const__))
#endif
#if __GNUC__ == 2 && __GNUC_MINOR__ >= 7
#define __unused    __attribute__((__unused__))
#define __noreturn  __attribute__((__noreturn__))
#define __noeffect  __attribute__((__const__))
#endif

/* 4. Makros for debugging and development */
#if !defined(TEST) && !defined(DEBUG)
#   define NDEBUG
#endif
#pragma pack()
#include <assert.h>
#pragma pack(2)

/* 5. Sytem-specific files */
#ifdef _AMIGA
#   include <aros/amiga.h>
#endif
#ifdef linux
#   include <aros/linux.h>
#endif
#ifdef _OSF1
#   include <aros/alpha.h>
#endif
#ifdef __FreeBSD__
#   include <aros/freebsd.h>
#endif
#ifdef __NetBSD__
#   include <aros/netbsd.h>
#endif
#ifdef __CYGWIN32__
#   include <aros/cygwin.h>
#endif

/* 4. Calculated #defines */
#if !AROS_STACK_GROWS_DOWNWARDS
#   define AROS_SLOWSTACKTAGS
#   define AROS_SLOWSTACKMETHODS
#endif /* !AROS_STACK_GROWS_DOWNWARDS */

#endif /* AROS_SYSTEM_H */
#pragma pack()

#ifndef AROS_SYSTEM_H
#define AROS_SYSTEM_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Analyse the current kind of system and compiler.
    Lang: english
*/

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

#if defined __GNUC__ && defined __GNUC_MINOR__
# define __GNUC_PREREQ(maj, min) \
        ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define __GNUC_PREREQ(maj, min) 0
#endif


/*
 * 2. Analyze compiler for STD C/C++.
 *
 * We test for the following:
 * a.	extern "C" linkage required for programs.
 * b.	inline, const, volatile, restrict keywords defined in
 *	newer C/C++ standards.
 * c.	throw() being available, which lets the compiler better
 *      optimize stuff
 */

#if defined(__cplusplus)
#   define EXTERN extern "C"
#   define BEGIN_EXTERN     extern "C" {
#   define END_EXTERN	    };
#   define __BEGIN_DECLS    extern "C" {
#   define __END_DECLS	    };
#else
#   define EXTERN extern
#   define BEGIN_EXTERN
#   define END_EXTERN
#   define __BEGIN_DECLS
#   define __END_DECLS
#endif

#if defined(__STDC__) || defined(__cplusplus)
#   define	    __const__	    const
#   define	    __inline__	    inline
#   define	    __volatile__    volatile

/*
 * C99 defines a new keyword restrict that can help do optimisation where
 * pointers are used in programs. We'd like to support optimisation :-)
 */
#   if defined(__STDC__VERSION__) &&  __STDC__VERSION__ >= 199901L
#	define	    __restrict__    restrict
#   else
#	define	    __restrict__
#	define	    restrict
#   endif

#else
#   define	    __const__
#   define	    const
#   define	    __inline__
#   define	    inline
#   define	    __volatile__
#   define	    volatile
#   define	    __restrict__
#   define	    restrict
#endif

#ifdef __GNUC__
#    if defined __cplusplus && __GNUC_PREREQ (2,8)
#        define __THROW       throw ()
#    else
#        define __THROW
#    endif
#endif

/* 3. Macros for making things more efficient */
#if __GNUC__ < 2 || __GNUC__ == 2 && __GNUC_MINOR__ < 5
#   define __unused
#   define __noreturn
#   define __noeffect
#endif
#if __GNUC__ == 2 && __GNUC_MINOR__ >= 5 && __GNUC_MINOR__ < 7
#   define __unused
#   define __noreturn  __attribute__((__noreturn__))
#   define __noeffect  __attribute__((__const__))
#endif
#if __GNUC__ == 2 && __GNUC_MINOR__ >= 7 || __GNUC__ > 2
#   define __unused    __attribute__((__unused__))
#   define __noreturn  __attribute__((__noreturn__))
#   define __noeffect  __attribute__((__const__))
#endif

/* 4. Makros for debugging and development */
#if !defined(__IDSTRING)
#   if defined(__GNUC__) && defined(__ELF__)
#       define __IDSTRING(name,str)	    __asm__(".ident\t\"" str "\"")
#   else
#       define __IDSTRING(name,str)	    static const char name[] __unused = str
#   endif
#endif

/* Need to protect __RCSID against the host system headers */
#if !defined(__RCSID)
#   define __RCSID(id)	__IDSTRING(rcsid,id)
#endif

/* __CONCAT is defined on Linux in cdefs.h */
#if !defined(__CONCAT)
#   if defined(__STDC__) || defined(__cplusplus)
#	define	    __CONCAT1(a,b)  a ## b
#	define	    __CONCAT(a,b)   __CONCAT1(a,b)
#   else
#	define	    __CONCAT(a,b)	a/**/b
#   endif
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#   define AROS_64BIT_TYPE long long
#   define AROS_HAVE_LONG_LONG
#endif

#if defined(__INTEL_COMPILER)
#   ifdef inline
#       undef inline
#   endif
#   define inline
#endif

#if defined __STDC__ && __STDC_VERSION__ >= 199901L
#   define AROS_HAVE_LONG_LONG
#endif

#if __GNUC__ <= 2
#   define __deprecated
#endif
#if __GNUC__ > 2
#   define __deprecated    __attribute__((__deprecated__))
#endif


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
#ifdef __OpenBSD__
#   include <aros/openbsd.h>
#endif
#ifdef __CYGWIN32__
#   include <aros/cygwin.h>
#endif

/* 4. Calculated #defines */
#if !AROS_STACK_GROWS_DOWNWARDS
#   define AROS_SLOWSTACKTAGS
#   define AROS_SLOWSTACKMETHODS
#endif /* !AROS_STACK_GROWS_DOWNWARDS */

#   define AROS_64BIT_TYPE long long

#ifndef AROS_ASMSYMNAME
#   define AROS_ASMSYMNAME(n)     n
#endif

#endif /* AROS_SYSTEM_H */

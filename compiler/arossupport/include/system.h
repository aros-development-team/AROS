#ifndef AROS_SYSTEM_H
#define AROS_SYSTEM_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Analyse the current kind of system
    Lang: english
*/
#ifndef _AROS
#   define _AROS
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

/* 2. Makros for debugging and development */
#if defined(TEST) || defined(DEBUG)
#   include <assert.h>
#else
#   define assert(x)        /* empty */
#endif

/* 3. Sytem-specific files */
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

/* 4. Calculated #defines */
#if !AROS_STACK_GROWS_DOWNWARDS
#   define AROS_SLOWSTACKTAGS
#   define AROS_SLOWSTACKMETHODS
#endif /* !AROS_STACK_GROWS_DOWNWARDS */

#endif /* AROS_SYSTEM_H */

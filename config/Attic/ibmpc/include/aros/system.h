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

/* 2. Analyze compiler */
#if defined(__cplusplus)
#   define EXTERN extern "C"
#   define BEGIN_EXTERN     extern "C" {
#   define END_EXTERN	    };
#else
#   define EXTERN extern
#   define BEGIN_EXTERN
#   define END_EXTERN
#endif

#endif /* AROS_SYSTEM_H */

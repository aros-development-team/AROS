/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef JMPDEFS_H
#define JMPDEFS_H

#include <aros/system.h>
#include <setjmp.h>

#if defined(__GLIBC__) && (__GLIBC__ >= 2)
#  define SP(env)	((APTR)(env[0].__jmpbuf[0].__sp))
#  define FP(env)	((APTR)(env[0].__jmpbuf[0].__fp))
#  define PC(env)	((APTR)(env[0].__jmpbuf[0].__pc))
#else
#  define SP(env)	((APTR)(env[0].__sp))
#  define FP(env)	((APTR)(env[0].__fp))
#  define PC(env)	((APTR)(env[0].__pc))
#endif

/* The number of stack longs that have to be copied from the old stack */
#if UseRegisterArgs
#  define NUM_LONGS	6
#else
#  define NUM_LONGS	4
#endif

#endif /* !JMPDEFS_H */

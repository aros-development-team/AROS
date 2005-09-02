/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef JMPDEFS_H
#define JMPDEFS_H

#include <setjmp.h>

#ifdef __linux__
#   if defined(__GLIBC__) && (__GLIBC__ >= 2)
#      define SP(env)	((APTR)(env[0].__jmpbuf[JB_GPR1]))
#      define FP(env)	(((APTR *)(env[0].__jmpbuf[JB_GPR1]))[0])
#      define PC(env)	((APTR)(env[0].__jmpbuf[JB_LR]))
#   else
#      define SP(env)	((APTR)(env[0].__sp))
#      define FP(env)	((APTR)(env[0].__bp))
#      define PC(env)	((APTR)(env[0].__pc))
#   endif
#endif
/* The number of stack longs that have to be copied from the old stack */
#define NUM_LONGS	2

#endif /* !JMPDEFS_H */

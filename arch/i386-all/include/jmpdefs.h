/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef JMPDEFS_H
#define JMPDEFS_H

#include <setjmp.h>

#ifdef __linux__
#   if defined(__GLIBC__) && (__GLIBC__ >= 2)
#      define SP(env)	(*(APTR*)&(env[0].__jmpbuf[JB_SP]))
#      define FP(env)	(*(APTR*)&(env[0].__jmpbuf[JB_BP]))
#      define PC(env)	(*(APTR*)&(env[0].__jmpbuf[JB_PC]))
#   else
#      define SP(env)	(*(APTR*)&(env[0].__sp))
#      define FP(env)	(*(APTR*)&(env[0].__bp))
#      define PC(env)	(*(APTR*)&(env[0].__pc))
#   endif
#endif
#ifdef __FreeBSD__
#   define SP(env)	(*(APTR*)&(env[0]._jb[2]))
#   define FP(env)	(*(APTR*)&(env[0]._jb[3]))
#   define PC(env)	(*(APTR*)&(env[0]._jb[0]))
#endif
#ifdef __NetBSD__
#   define SP(env)      (*(APTR*)&(env[2]))
#   define FP(env)      (*(APTR*)&(env[3]))
#   define PC(env)      (*(APTR*)&(env[0]))
#endif
#ifdef __OpenBSD__
#   define SP(env)      (*(APTR*)&(env[2]))
#   define FP(env)      (*(APTR*)&(env[3]))
#   define PC(env)      (*(APTR*)&(env[0]))
#endif
/* The number of stack longs that have to be copied from the old stack */
#define NUM_LONGS	4

#endif /* !JMPDEFS_H */

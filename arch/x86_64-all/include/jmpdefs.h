/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: jmpdefs.h 21223 2004-03-15 08:17:05Z iaint $
*/

#ifndef JMPDEFS_H
#define JMPDEFS_H

#include <setjmp.h>

#ifdef __linux__
#   if defined(__GLIBC__) && (__GLIBC__ >= 2)
#      define SP(env)	(*(APTR*)&(env[0].__jmpbuf[JB_RSP]))
#      define FP(env)	(*(APTR*)&(env[0].__jmpbuf[JB_RBP]))
#      define PC(env)	(*(APTR*)&(env[0].__jmpbuf[JB_PC]))
#   else
#      define SP(env)	(*(APTR*)&(env[0].__rsp))
#      define FP(env)	(*(APTR*)&(env[0].__rbp))
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

#define _JMPLEN		15 /* or 8 on i386 ??? */

#endif /* !JMPDEFS_H */

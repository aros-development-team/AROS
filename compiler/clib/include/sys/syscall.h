#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: syscalls definitions
    Lang: english
*/

#include <exec/types.h>
#include <exec/libraries.h>

enum
{
#define SYSTEM_CALL(name) ,SYS_##name
SYS_clibdummy = LIB_RESERVED
#include <sys/syscall.def>
#undef SYSTEM_CALL
};

#ifdef __GNUC__
#   define syscall(name,args...)				    \
    ({								    \
	extern struct Library *aroscbase;			    \
	register int (*_sc)() = __AROS_GETVECADDR(aroscbase, name); \
	_sc (args);						    \
    })
#else
/* Force an error! When syscall() is used*/
#   define syscall(name,args...)    SYSCALL_BROKEN
#endif

#endif /* _SYSCALL_H_ */

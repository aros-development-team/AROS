#ifndef _SYSCALL_H
#define _SYSCALL_H

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

#define syscall(name,args...) ({ register int (*_sc)() = __AROS_GETVECADDR(aroscbase, SYS_##name); _sc (args) ;})

#endif /* _SYSCALL_H */

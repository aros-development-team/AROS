#ifndef _SYSCALL_H
#define _SYSCALL_H

/*
    (C) 1997-98 AROS - The Amiga Research OS
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
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
#define SYSTEM_CALL(name, x...) ,SYS_##name
SYS_clibdummy = LIB_RESERVED
#include <sys/syscall.def>
#undef SYSTEM_CALL
};

extern struct Library *aroscbase;			   
#define syscall(name,args...) \
    ((int (*)())__AROS_GETVECADDR(aroscbase, name))(args)

#endif /* _SYSCALL_H_ */

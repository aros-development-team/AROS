/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for misc.resource
    Lang: English
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif
#ifndef RESOURCES_MISC_H
#include <resources/misc.h>
#endif

struct MiscBase
{
    struct Node		    mb_Node;
    struct ExecBase	   *mb_SysBase;

    struct SignalSemaphore  mb_Lock;

    char                   *mb_owners[MR_MAXUNIT];
};

#ifdef  SysBase
#undef  SysBase
#endif
#define SysBase		(GPB(MiscBase)->mb_SysBase)

#define GPB(x)          ((struct MiscBase *)x)


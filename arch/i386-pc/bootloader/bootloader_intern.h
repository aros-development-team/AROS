/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Internal data structures for bootloader.resource
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif
#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif
#include <aros/bootloader.h>

struct BootLoaderBase
{
    struct Node		 bl_Node;
    struct ExecBase	*bl_SysBase;
    struct Library	*bl_UtilityBase;
    ULONG		 Flags;
    STRPTR		 LdrName;
    struct List		 Args;
    struct VesaInfo	 Vesa;
};

#define SysBase		(BootLoaderBase->bl_SysBase)
#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase	(BootLoaderBase->bl_UtilityBase)

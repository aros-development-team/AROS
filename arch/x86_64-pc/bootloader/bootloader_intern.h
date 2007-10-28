/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: bootloader_intern.h 24260 2006-04-05 20:16:56Z verhaegs $

    Internal data structures for bootloader.resource
*/

#ifndef BOOTLOADER_INTERN_H
#define BOOTLOADER_INTERN_H

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
    ULONG		 Flags;
    STRPTR		 LdrName;
    struct List		 Args;
    struct List		 DriveInfo;
    struct VesaInfo	 Vesa;
};

#endif //BOOTLOADER_INTERN_H

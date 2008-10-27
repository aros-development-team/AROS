/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: bootloader_intern.h 29778 2008-10-16 19:40:21Z neil $

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
    STRPTR		 LdrName;
    struct List		 Args;
};

#endif //BOOTLOADER_INTERN_H

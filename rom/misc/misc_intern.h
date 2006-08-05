/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for misc.resource
    Lang: English
*/

#ifndef MISC_INTERN_H
#define MISC_INTERN_H

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

#include <exec/libraries.h>

struct MiscBase
{
    struct Node             mb_Node;

    struct SignalSemaphore  mb_Lock;

    char                   *mb_owners[MR_MAXUNIT];
};

#define GPB(x)          ((struct MiscBase *)x)

#endif //MISC_INTERN_H

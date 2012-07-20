/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef TASKSTORAGE_H
#define TASKSTORAGE_H

#include <exec/nodes.h>
#include "etask.h"

struct TaskStorageFreeSlot
{
    struct MinNode _node;
    int FreeSlot;
};

#define __TS_FIRSTSLOT 0

#endif /* TASKSTORAGE_H */

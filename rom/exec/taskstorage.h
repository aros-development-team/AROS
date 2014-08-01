/*
    Copyright � 2011-2014, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef TASKSTORAGE_H
#define TASKSTORAGE_H

#include <exec/nodes.h>
#include "etask.h"

struct TaskStorageFreeSlot
{
    struct MinNode _node;
    LONG FreeSlot;
};

#define __TS_FIRSTSLOT 0

IPTR TaskGetStorageSlot(struct Task * t, LONG id);

#endif /* TASKSTORAGE_H */

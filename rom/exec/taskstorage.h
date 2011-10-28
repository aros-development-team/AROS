/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef TASKSTORAGE_H
#define TASKSTORAGE_H

#include <exec/nodes.h>

struct TaskStorageFreeSlot
{
    struct MinNode _node;
    int FreeSlot;
};

#endif /* TASKSTORAGE_H */

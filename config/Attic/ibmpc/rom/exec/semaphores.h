/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1999/02/13 15:08:44  schulz
    Here goes whole ibmpc/rom directory.

    Revision 1.2  1996/08/01 17:41:27  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include <exec/tasks.h>
#include <exec/nodes.h>

/* Signal flag to awake tasks waiting on a semaphore */
#define SEMAPHORESIGF (0x8000)

/*
    Node for a task waiting synchronously:
    Maybe this shouldn't have a struct Node as first element - but I want
    it to be compatible to Procure()'s struct SemaphoreMessage.
    This means that ln_Name contains the lock type (SM_EXCLUSIVE or
    SM_SHARED). And since ln_Type is NT_MESSAGE for semaphore messages
    waiting on the semaphore list it isn't free either. The only field
    left in the node is ln_Pri which contains the node type (see below).
*/
struct SemaphoreNode
{
    struct Node node;
    struct Task *task;
};

/* Node types in the semaphore's waiting queue */
#define SN_TYPE_OBTAIN	0
#define SN_TYPE_PROCURE 1

#endif

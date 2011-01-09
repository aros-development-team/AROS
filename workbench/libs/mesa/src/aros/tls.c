/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "tls.h"

#include <proto/exec.h>

struct TaskLocalNode
{
    struct TaskLocalNode    * tl_Succ;
    struct Task             * tl_Task;
    APTR                    tl_Data;
};

struct TaskLocalStorage
{
    struct TaskLocalNode    * tls_Head;
    struct SignalSemaphore  tls_WriteSemaphore;
};

/* Implementation uses locking only when adding objects. Objects are always added
   at head. The list is never reordered, thus reading can be done without locking */

/* Idea: can consider adding list trimming when holding insert lock: instead of
   adding an object to head of existing list, a new list would be created where
   object would be copies (not pointer to!) of objects on current list. During
   copying objects with NULL data items would not be copied. Once copying done,
   new list would be put in place of old list. The old list however would then
   leak. Maybe this leak could be fixed by implementing smart pointers/reference
   counting on TaskLocalNode - this could however make iterations slowe */

struct TaskLocalStorage * CreateTLS()
{
    struct TaskLocalStorage * tls = 
        AllocVec(sizeof(struct TaskLocalStorage), MEMF_PUBLIC | MEMF_CLEAR);

    InitSemaphore(&tls->tls_WriteSemaphore);
    tls->tls_Head = NULL;

    return tls;
}

VOID InsertIntoTLS(struct TaskLocalStorage * tls, APTR ptr)
{
    struct TaskLocalNode * tl = tl = tls->tls_Head;
    struct Task * me = FindTask(NULL);
    struct TaskLocalNode * selected = NULL;
    
    /* Assumption: one task cannot be reviewing the list and adding the head
       "at the same time" - do not alter this function to recurse */
    /* Assumption: only task A can add entry for task A */

    /* Check if task's storage is already on the list */    
    while(tl)
    {
        if (tl->tl_Task == me)
        {
            selected = tl;
            break;
        }
        tl = tl->tl_Succ;
    }
    
    if (!selected)
    {
        /* No, it is not. Create, set task pointer and at to head of list */
        selected = AllocVec(sizeof(struct TaskLocalNode), MEMF_PUBLIC | MEMF_CLEAR);
        selected->tl_Task = me;
        ObtainSemaphore(&tls->tls_WriteSemaphore);
        selected->tl_Succ = tls->tls_Head;
        tls->tls_Head = selected;
        ReleaseSemaphore(&tls->tls_WriteSemaphore);
        
    }

    /* Set the passed value */
    selected->tl_Data = ptr;
}

APTR GetFromTLS(struct TaskLocalStorage * tls)
{
    struct TaskLocalNode * tl = tls->tls_Head;
    struct Task * me = FindTask(NULL);
    APTR data = NULL;
    
    while(tl)
    {
        if (tl->tl_Task == me)
        {
            data = tl->tl_Data;
            break;
        }
        tl = tl->tl_Succ;
    }

    return data;
}

VOID DestroyTLS(struct TaskLocalStorage * tls)
{
    /* Destroy needs no lock. If a task is still iterating over list, we are doomed
       anyway */
    struct TaskLocalNode * tl = tls->tls_Head, * temp;

    while (tl)
    {
        temp = tl->tl_Succ;
        FreeVec(tl);
        tl = temp;
    }

    FreeVec(tls);
}

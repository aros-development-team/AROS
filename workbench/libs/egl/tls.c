/*
    Copyright 2010-2019, The AROS Development Team. All rights reserved.
    $Id: tls.c 56280 2019-04-17 18:42:57Z NicJA $
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

/* This approach is used to achieve acceptable performance. With semaphore-locking
   of read path, the performance was degraded several times. The TLS is used to
   hold current per-task GL context - retrieving this context MUST BE fast */

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

VOID ClearFromTLS(struct TaskLocalStorage * tls)
{
    /* Clearing is inserting a NULL. Element can't be removed from list - since
       there is no read locking, altering structure of list when other tasks
       are reading it, would cause crashes */
    /* TODO: How real clearing can be achieved:
     * a) acquire write lock
     * b) copy all element (copy not relink!) from _current list to _new list except for the element that
     *    is beeing cleared
     * c) _old = _current, _current = _new
     * d) release write lock
     *
     * How to delete _old? It can't be deleted right away, because some read tasks can be iterating over it.
     * a) put it on garbage collect list (the whole list, not relinking nodes!) and clear it when shutting down
     * b) use memory pool to allocate all TLS objects and clear pool at shutdown
     *
     * Solution b is much easier and convenient
     */
    InsertIntoTLS(tls, NULL);
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

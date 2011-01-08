/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "tls.h"

#include <proto/exec.h>

struct TaskLocalStorage
{
    struct List             tls_TaskLocalList;
    struct SignalSemaphore  tls_Semaphore;
};

struct TaskLocalNode
{
    struct Node     tl_Node;
    struct Task *   tl_Task;
    APTR            tl_Data;
};

struct TaskLocalStorage * CreateTLS()
{
    struct TaskLocalStorage * tls = 
        AllocVec(sizeof(struct TaskLocalStorage), MEMF_PUBLIC | MEMF_CLEAR);

    InitSemaphore(&tls->tls_Semaphore);
    NEWLIST(&tls->tls_TaskLocalList);

    return tls;
}

VOID InsertIntoTLS(struct TaskLocalStorage * tls, APTR ptr)
{
    struct TaskLocalNode * tl;
    struct Task * me = FindTask(NULL);
    struct TaskLocalNode * selected = NULL;
    
    ObtainSemaphore(&tls->tls_Semaphore);
    ForeachNode(&tls->tls_TaskLocalList, tl)
    {
        if (tl->tl_Task == me)
        {
            selected = tl;
            break;
        }
    }
    
    if (!selected)
    {
        /* Create, set values, add */
        selected = AllocVec(sizeof(struct TaskLocalNode), MEMF_PUBLIC | MEMF_CLEAR);
        selected->tl_Task = me;
        ADDHEAD(&tls->tls_TaskLocalList, selected);
    }
    
    selected->tl_Data = ptr;
    
    ReleaseSemaphore(&tls->tls_Semaphore);
    
    /* TODO: When to delete an existing object? When NULL is passed as t? */
}

APTR GetFromTLS(struct TaskLocalStorage * tls)
{
    struct TaskLocalNode * tl;
    struct Task * me = FindTask(NULL);
    APTR data = NULL;
    
    ObtainSemaphore(&tls->tls_Semaphore);
    ForeachNode(&tls->tls_TaskLocalList, tl)
    {
        if (tl->tl_Task == me)
        {
            data = tl->tl_Data;
            break;
            /* TODO: be smart? if list long, move the found node to beginning? */
        }
    }
    ReleaseSemaphore(&tls->tls_Semaphore);
    
    return data;
}

VOID DestroyTLS(struct TaskLocalStorage * tls)
{
    struct TaskLocalNode * n, *m;
    ObtainSemaphore(&tls->tls_Semaphore);
    ForeachNodeSafe(&tls->tls_TaskLocalList, n, m)
    {
        Remove(&n->tl_Node);
        FreeVec(n);
    }
    ReleaseSemaphore(&tls->tls_Semaphore);
    FreeVec(tls);
}

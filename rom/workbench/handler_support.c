/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Support functions for working with the handler.
*/

#include "workbench_intern.h"
#include "handler.h"
#include "handler_support.h"

BOOL __StartHandler(struct WorkbenchBase *WorkbenchBase)
{
    struct Task    *thisTask = FindTask(NULL);
    struct Process *proc     = NULL;
    
    /*
        Protect against a race-conditition when starting the handler. It's 
        possible that multiple tasks are trying to start it at the same time.
    */
    ObtainSemaphore(&WorkbenchBase->wb_HandlerSemaphore);

    /* 
        Someone might have obtained the semaphore before us, so the handler 
        might already be started when we obtain it. Just be happy and return 
        success.
    */
    if (WorkbenchBase->wb_HandlerPort != NULL)
    {
        ReleaseSemaphore(&WorkbenchBase->wb_HandlerSemaphore);
        return TRUE;
    }
    
    /* 
        Start the handler. It will set wb_HandlerPort itself when it is ready
        to accept messages, we don't do it here.
    */
    proc = CreateNewProcTags
    (
        NP_Entry,     (IPTR) WorkbenchHandler,
        NP_StackSize,        8129,
        NP_Name,      (IPTR) "Workbench Handler",
        NP_UserData,  (IPTR) WorkbenchBase,
        TAG_DONE
    );
    
    if (proc ==  NULL)
    {
        ReleaseSemaphore(&WorkbenchBase->wb_HandlerSemaphore);
        return FALSE;
    }
    
    /*
        Wait until the handler is ready to accept messages. This is basically
        a busy-loop, but since we simply call Switch() all the time, we should
        only take CPU time when the system is idle (and a little overhead).
    */
    while
    (
           WorkbenchBase->wb_HandlerPort  == NULL 
        && WorkbenchBase->wb_HandlerError == 0
    )
    {
        SetTaskPri(thisTask, thisTask->tc_Node.ln_Pri);
    }
    
    ReleaseSemaphore(&WorkbenchBase->wb_HandlerSemaphore);
    
    if (WorkbenchBase->wb_HandlerError != 0)
    {
        /* Clear the error code */
        WorkbenchBase->wb_HandlerError = 0; /* FIXME: should be propagated */
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

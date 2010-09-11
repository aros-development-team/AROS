/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "workbench_intern.h"
#include "handler.h"
#include "handler_support.h"

struct WBCommandMessage *__CreateWBCM_WB
(
    enum WBCM_Type type, struct WorkbenchBase *WorkbenchBase
)
{
    struct WBCommandMessage *message = (struct WBCommandMessage *) AllocMem
    (
        WBCM_SIZE, MEMF_PUBLIC | MEMF_CLEAR
    );
    
    if (message != NULL)
    {
        message->wbcm_Message.mn_Length = WBCM_SIZE;
        message->wbcm_Type = type;
    }
    
    return message;
}

VOID __DestroyWBCM_WB
(
    struct WBCommandMessage *message, struct WorkbenchBase *WorkbenchBase
)
{
    if (message != NULL)
    {
        FreeMem(message, WBCM_SIZE);
    }
}

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "support_messages.h"

/*** Generic message handling functions *************************************/
struct Message *__AllocMessage_WB
(
    ULONG size, struct WorkbenchBase *WorkbenchBase
)
{
    struct Message *message = (struct Message *) AllocMem
    (
        size, MEMF_PUBLIC | MEMF_CLEAR
    );
    
    if (message != NULL)
    {
        message->mn_Length = size;
    }
    
    return message;
}

VOID __FreeMessage_WB
(
    struct Message *message, struct WorkbenchBase *WorkbenchBase
)
{
    if (message != NULL)
    {
        FreeMem(message, message->mn_Length);
    }
}

/*** Specialized message handling functions *********************************/
/*== WBStartup =============================================================*/
struct WBStartup *__CreateWBS_WB(struct WorkbenchBase *WorkbenchBase)
{
    return (struct WBStartup *) AllocMessage(WBS_SIZE);
}

VOID __DestroyWBS_WB
(
    struct WBStartup *message, struct WorkbenchBase *WorkbenchBase
)
{
    if (message != NULL)
    {
        struct WBArg *args = message->sm_ArgList;
        ULONG         i;
        
        for (i = 0; i < message->sm_NumArgs; i++)
        {
            if (args[i].wa_Lock != NULL) UnLock(args[i].wa_Lock);
            if (args[i].wa_Name != NULL) FreeVec(args[i].wa_Name);
        }
        
        FreeMessage((struct Message *) message);
    }
}

/*== WBHandlerMessage ======================================================*/
struct WBHandlerMessage *__CreateWBHM_WB
(
    enum WBHM_Type type, struct WorkbenchBase *WorkbenchBase
)
{
    struct WBHandlerMessage *message = (struct WBHandlerMessage *) AllocMessage(WBHM_SIZE);
    
    if (message != NULL)
    {
        message->wbhm_Type = type;
    }
    
    return message;
}

VOID __DestroyWBHM_WB
(
    struct WBHandlerMessage *message, struct WorkbenchBase *WorkbenchBase
)
{
    if (message != NULL)
    {
        switch (message->wbhm_Type)
        {
            case WBHM_TYPE_OPEN:
                FreeVec(message->wbhm_Data.Open.Name);
                break;
                
            case WBHM_TYPE_UPDATE:
                // FIXME
                break;
                
            case WBHM_TYPE_SHOW:
            case WBHM_TYPE_HIDE:
            default:
                /* No additional resources to free */
        }
    
        FreeMessage((struct Message *) message);
    }
}

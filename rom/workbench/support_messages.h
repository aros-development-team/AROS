#ifndef _SUPPORT_MESSAGES_H_
#define _SUPPORT_MESSAGES_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "workbench_intern.h"

/*** Macros and definitions *************************************************/
#define WBS_SIZE  (sizeof(struct WBStartup))
#define WBS(msg)  ((struct WBStartup *) (msg))

/*** Prototypes *************************************************************/
struct Message *__AllocMessage_WB(ULONG size, struct WorkbenchBase *WorkbenchBase);
VOID __FreeMessage_WB(struct Message *message, struct WorkbenchBase *WorkbenchBase);

struct WBStartup *__CreateWBS_WB(struct WorkbenchBase *WorkbenchBase);
VOID __DestroyWBS_WB(struct WBStartup *startup, struct WorkbenchBase *WorkbenchBase);
struct WBHandlerMessage *__CreateWBHM_WB(enum WBHM_Type type, struct WorkbenchBase *WorkbenchBase);
VOID __DestroyWBHM_WB(struct WBHandlerMessage *message, struct WorkbenchBase *WorkbenchBase);

/*** Function macros ********************************************************/
#define AllocMessage(size) (__AllocMessage_WB((size), LB(WorkbenchBase)))
#define FreeMessage(msg)   (__FreeMessage_WB((msg), LB(WorkbenchBase)))

#define CreateWBS()        (__CreateWBS_WB(LB(WorkbenchBase)))
#define DestroyWBS(msg)    (__DestroyWBS_WB((msg), LB(WorkbenchBase)))
#define CreateWBHM(type)   (__CreateWBHM_WB((type), LB(WorkbenchBase)))
#define DestroyWBHM(msg)   (__DestroyWBHM_WB((msg), LB(WorkbenchBase)))

#endif /* _SUPPORT_MESSAGES_H_ */

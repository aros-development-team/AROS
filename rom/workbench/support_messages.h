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
struct IntWBHandlerMessage *__CreateIWBHM_WB(enum WBHM_Type type, struct MsgPort *replyport, struct WorkbenchBase *WorkbenchBase);
VOID __DestroyIWBHM_WB(struct IntWBHandlerMessage *message, struct WorkbenchBase *WorkbenchBase);

/*** Function macros ********************************************************/
#define AllocMessage(size) __AllocMessage_WB((size), LB(WorkbenchBase))
#define FreeMessage(msg)   __FreeMessage_WB((msg), LB(WorkbenchBase))

#define CreateWBS()              __CreateWBS_WB(LB(WorkbenchBase))
#define DestroyWBS(msg)          __DestroyWBS_WB((msg), LB(WorkbenchBase))
#define CreateIWBHM(type, rport) __CreateIWBHM_WB((type), (rport), (LB(WorkbenchBase)))
#define DestroyIWBHM(msg)        __DestroyIWBHM_WB((msg), LB(WorkbenchBase))
#define CreateWBHM(type)         (&CreateIWBHM((type), NULL)->iwbhm_wbhm)
#define DestroyWBHM(msg)         DestroyIWBHM((struct IntWBHandlerMessage *)msg)

#endif /* _SUPPORT_MESSAGES_H_ */

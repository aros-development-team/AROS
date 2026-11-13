#ifndef _HANDLER_SUPPORT_H_
#define _HANDLER_SUPPORT_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "workbench_intern.h"

/*** Prototypes *************************************************************/
struct WBCommandMessage *__CreateWBCM_WB(enum WBCM_Type type, struct WorkbenchBase *WorkbenchBase);
VOID __DestroyWBCM_WB(struct WBCommandMessage *message, struct WorkbenchBase *WorkbenchBase);

/*** Macros *****************************************************************/
#define CreateWBCM(type) (__CreateWBCM_WB((type), LB(WorkbenchBase)))
#define DestroyWBCM(message) (__DestroyWBCM_WB((message), LB(WorkbenchBase)))

#endif /* _HANDLER_SUPPORT_H_ */

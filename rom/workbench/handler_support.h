#ifndef _HANDLER_SUPPORT_H_
#define _HANDLER_SUPPORT_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Support functions for working with the handler.
*/

#include <exec/types.h>
#include "workbench_intern.h"

/*** Prototypes ************************************************************/
BOOL __StartHandler(struct WorkbenchBase *WorkbenchBase);

/*** Macros *****************************************************************/
#define StartHandler() (__StartHandler(WorkbenchBase))

#endif /* _HANDLER_SUPPORT_H_ */

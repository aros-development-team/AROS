/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Headers for the misc support functions.
    Lang: English
*/

#ifndef __WORKBENCH_SUPPORT_H__
#define __WORKBENCH_SUPPORT_H__

#include "workbench_intern.h"

/*** Prototypes ************************************************************/
BOOL StartHandler(struct WorkbenchBase *WorkbenchBase);

void AddHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase);
void RemoveHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase);


#endif /* __WORKBENCH_SUPPORT_H__ */

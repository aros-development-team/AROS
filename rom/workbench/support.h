/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Headers for the miscellanous support functions.
*/

#ifndef __WORKBENCH_SUPPORT_H__
#define __WORKBENCH_SUPPORT_H__

#include "workbench_intern.h"

/*** Prototypes ************************************************************/
BOOL   __StartHandler(struct WorkbenchBase *WorkbenchBase);
VOID   __AddHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase);
VOID   __RemoveHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase);
STRPTR __AllocateNameFromLock(BPTR lock, struct WorkbenchBase *WorkbenchBase);
STRPTR __StrDup(CONST_STRPTR string, struct WorkbenchBase *WorkbenchBase);

/*** Macros *****************************************************************/
#define StartHandler() (__StartHandler(WorkbenchBase))
#define AddHiddenDevice(name) (__AddHiddenDevice((name), WorkbenchBase))
#define RemoveHiddenDevice(name) (__RemoveHiddenDevice((name), WorkbenchBase))
#define AllocateNameFromLock(lock) (__AllocateNameFromLock((lock), WorkbenchBase))
#define StrDup(string) (__StrDup((string), WorkbenchBase))

#endif /* __WORKBENCH_SUPPORT_H__ */

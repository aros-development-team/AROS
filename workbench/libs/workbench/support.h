#ifndef __WORKBENCH_SUPPORT_H__
#define __WORKBENCH_SUPPORT_H__

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Headers for the miscellanous support functions.
*/

#include "workbench_intern.h"

/*** Prototypes ************************************************************/
VOID   __AddHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase);
VOID   __RemoveHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase);
STRPTR __AllocateNameFromLock(BPTR lock, struct WorkbenchBase *WorkbenchBase);
STRPTR __StrDup(CONST_STRPTR string, struct WorkbenchBase *WorkbenchBase);
BPTR   __DuplicateSearchPath(BPTR list, struct WorkbenchBase *WorkbenchBase);
VOID   __FreeSearchPath(BPTR list, struct WorkbenchBase *WorkbenchBase);

/*** Macros *****************************************************************/
#define AddHiddenDevice(name) (__AddHiddenDevice((name), WorkbenchBase))
#define RemoveHiddenDevice(name) (__RemoveHiddenDevice((name), WorkbenchBase))
#define AllocateNameFromLock(lock) (__AllocateNameFromLock((lock), WorkbenchBase))
#define StrDup(string) (__StrDup((string), WorkbenchBase))
#define DuplicateSearchPath(list) (__DuplicateSearchPath((list), WorkbenchBase))
#define FreeSearchPath(list) (__FreeSearchPath((list), WorkbenchBase))

#endif /* __WORKBENCH_SUPPORT_H__ */

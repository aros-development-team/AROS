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

extern BOOL StartHandler( struct WorkbenchBase *WorkbenchBase );

extern void AddHiddenDevice( struct List *deviceList, STRPTR name );
extern void RemoveHiddenDevice( struct List *deviceList, STRPTR name );

#endif /* __WORKBENCH_SUPPORT_H__ */

/*
    (C) 2000 AROS - The Amiga Research OS
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
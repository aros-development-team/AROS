/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Miscellanous support functions.
*/

#define DEBUG 1

#include <dos/dostags.h>

#include "workbench_intern.h"
#include "handler.h"

BOOL StartHandler(struct WorkbenchBase *WorkbenchBase)
{
    struct Process *proc;
    struct TagItem  procTags[] =
    {
        { NP_Entry,      (IPTR) WorkbenchHandler    },
        { NP_StackSize,  8192                       },
        { NP_Name,       (IPTR) "Workbench Handler" },
        { TAG_DONE,      NULL                       }
    };

    if ((proc = CreateNewProc( procTags )))
    {
        D(bug("Workbench: Started Workbench Handler.\n"));
        return TRUE;
    }
    else
    {
        D(bug("Workbench: Could not start Workbench Handler!\n"));
        return FALSE;
    }
}

void AddHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase)
{
    /* Make sure we got valid pointers... */
    if ((name == NULL) || (WorkbenchBase == NULL))
    {
        D(bug("Workbench/AddHiddenDevice: Got NULL pointers!\n"));
        return;
    }

    /* Only add the device name if it isn't already the list. */
    if (FindName(&(WorkbenchBase->wb_HiddenDevices), name ) == NULL)
    {
        struct Node *deviceName;

        if ((deviceName = AllocMem(sizeof(struct Node), MEMF_ANY | MEMF_CLEAR )))
        {
            deviceName->ln_Name = name;
            AddTail(&(WorkbenchBase->wb_HiddenDevices), deviceName);

            /* FIXME: Notify WB App. Not here though. (We might want to use this
             * onn startup for adding all hidden devices that was set in prefs,
             * then unneccesery to notify app at each addition... */
        }
    }
}

void RemoveHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase)
{
    struct Node *deviceName;

    /* Make sure we got valid pointers... */
    if ((name == NULL) || (WorkbenchBase == NULL))
    {
        D(bug("Workbench/RemoveHiddenDevice: Got NULL pointers!\n"));
        return;
    }

    if ((deviceName = FindName(&(WorkbenchBase->wb_HiddenDevices), name)))
    {
        Remove(deviceName);
        FreeVec(deviceName);

        /* TODO: Notify WB App. Maybe not here...*/
    }
}

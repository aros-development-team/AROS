/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Miscellanous support functions.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <aros/atomic.h>
#include <dos/dostags.h>
#include <string.h>

#include "workbench_intern.h"
#include "support.h"

void __AddHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase)
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

void __RemoveHiddenDevice(STRPTR name, struct WorkbenchBase *WorkbenchBase)
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

STRPTR __AllocateNameFromLock(BPTR lock, struct WorkbenchBase *WorkbenchBase)
{
    ULONG  length = 512;
    STRPTR buffer = NULL;
    BOOL   done   = FALSE;
    
    while (!done)
    {
        if (buffer != NULL) FreeVec(buffer);
        
        buffer = AllocVec(length, MEMF_ANY);
        if (buffer != NULL)
        {
            if (NameFromLock(lock, buffer, length))
            {
                done = TRUE;
                break;
            }
            else
            {
                if (IoErr() == ERROR_LINE_TOO_LONG)
                {
                    length += 512;
                    continue;
                }
                else
                {
                    break;
                }                
            }
        }
        else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            break;
        }
    }
    
    if (done)
    {
        return buffer;
    }
    else
    {
        if (buffer != NULL) FreeVec(buffer);
        return NULL;
    }
}

STRPTR __StrDup(CONST_STRPTR str, struct WorkbenchBase *WorkbenchBase)
{
    STRPTR dup;
    ULONG  len;

    if (str == NULL) return NULL;
    
    len = strlen(str);
    dup = AllocVec(len + 1, MEMF_PUBLIC);
    if (dup != NULL) CopyMem(str, dup, len + 1);
    
    return dup;
}

/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Misc support functions.
    Lang: English
*/

#include <dos/dostags.h>

#include "workbench_intern.h"
#include "handler.h"

BOOL StartHandler( struct WorkbenchBase *WorkbenchBase ) {
    struct Process *proc;
    struct TagItem  procTags[] = {
        { NP_Entry,      (IPTR) WorkbenchHandler    },
        { NP_StackSize,  8192                       },
        { NP_Name,       (IPTR) "Workbench Handler" },
        { TAG_DONE,      NULL                       }
    };

    kprintf( "Workbench: About to star WBHandler...\n" );
    proc = CreateNewProc( procTags );
    kprintf( "Workbench: ...DONE!\n" );

    if( proc ) {
        kprintf( "Workbench: Started Workbench Handler.\n" );
        WorkbenchBase->wb_HandlerPort = &(proc->pr_MsgPort);

        return TRUE;
    } else {
        kprintf( "Workbench: Starting Workbench Handler failed!\n" );
        return FALSE;
    }
}

void AddHiddenDevice( STRPTR name, struct WorkbenchBase *WorkbenchBase ) {
    /* Make sure we got valid pointers... */
    if( (name == NULL) || (WorkbenchBase == NULL) ) {
        return;
    }

    /* Only add the device name if it isn't already the list. */
    if( FindName( &(WorkbenchBase->wb_HiddenDevices), name ) == NULL ) {
        struct Node *deviceName;

        if( (deviceName = AllocMem( sizeof( struct Node ), MEMF_ANY | MEMF_CLEAR )) ) {
            deviceName->ln_Name = name;
            AddTail( &(WorkbenchBase->wb_HiddenDevices), deviceName );

            /* TODO: Notify WB App. */
        }
    }
}

void RemoveHiddenDevice( STRPTR name, struct WorkbenchBase *WorkbenchBase ) {
    struct Node *deviceName;

    /* Make sure we got valid pointers... */
    if( (name == NULL) || (WorkbenchBase == NULL) ) {
        return;
    }

    if( (deviceName = FindName( &(WorkbenchBase->wb_HiddenDevices), name )) ) {
        Remove( deviceName );
        FreeVec( deviceName );

        /* TODO: Notify WB App. Maybe not here...*/
    }
}
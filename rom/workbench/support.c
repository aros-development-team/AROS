/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Misc support functions.
    Lang: English
*/

#include <dos/dostags.h>

#include "workbench_intern.h"

BOOL StartHandler( WorkbenchBase *WorkbenchBase ) {
    struct Process *proc;
    struct TagItem  procTags[] = {
        { NP_Entry,      (IPTR) WorkbenchHandler    },
        { NP_StackSize,  8192                       },
        { NP_Name,       (IPTR) "Workbench Handler" },
        { TAG_DONE,      NULL                       }
    };

    kprintf( "Workbench: About to star WBHandler...\n" );
    handler = CreateNewProc( procTags );
    kprintf( "Workbench: ...DONE!\n" );

    if( handler ) {
        kprintf( "Workbench: Started Workbench Handler.\n" );
        WorkbenchBase->wb_HandlerPort = &handler->pr_MsgPort;

        return TRUE;
    } else {
        kprintf( "Workbench: Starting Workbench Handler failed!\n" );
        return FALSE;
    }
}
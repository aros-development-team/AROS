/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: The Workbench Handler process and associated functions
    Lang: English
*/


#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <dos/dosextens.h>

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>
#include <workbench/startup.h>

/* This is the main entry point for the Workbench Handler process. */

void WorkbenchHandler( void ) {
    struct WorkbenchBase *WorkbenchBase;

    struct Process       *process = (struct Process *) FindTask( NULL );
    struct MsgPort       *port    = &(process->pr_MsgPort);

    BOOL                  running = TRUE;
    struct IntuiMessage  *message;

    kprintf( "WBHandler: I'm alive! Alive I tell you!\n" );

    /* First of all, open Workbench library so we can get at stuff. */
    if( !(WorkbenchBase = (struct WorkbenchBase *) OpenLibrary( WORKBENCHNAME, 37L )) ) {
        return;
    }

    /* Notify Intuition that we are alive. */
    AlohaWorkbench( port );

    /* Main event loop */
    while( running ) {
        WaitPort( port );

        while( (message = (struct IntuiMessage *) GetMsg( port )) != NULL ) {
            if( message->Class == IDCMP_WBENCHMESSAGE ) {
                switch( message->Code ) {
                    case WBENCHOPEN:
                        /* Notify WB apps to open windows */
                        break;

                    case WBENCHCLOSE:
                        /* Notify WB apps to close windows */
                        break;

                    /* TODO: More messages... */
                }
            }

            ReplyMsg( (struct Message *) message );
        }
    }

    /* Shutting down... */
    AlohaWorkbench( NULL );

    if( WorkbenchBase ) {
        CloseLibrary( (struct Library *) WorkbenchBase );
    }
}
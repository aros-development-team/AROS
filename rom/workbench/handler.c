/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The Workbench Handler process and associated functions
    Lang: English
*/

#define DEBUG 1

#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <dos/dosextens.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/workbench.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <workbench/workbench.h>
#include <workbench/startup.h>

#include "workbench_intern.h"

/* This is the main entry point for the Workbench Handler process. */

void WorkbenchHandler( void ) {
    struct WorkbenchBase *WorkbenchBase;

    struct MsgPort       *port;

    ULONG                 openCount = 0;
    struct WBStartup     *incoming;
    struct WBStartup     *outgoing;

    D(bug( "WBHandler: I'm alive! Alive I tell you!\n" ));

    /* First of all, we need to open workbench.library. */
    if( !(WorkbenchBase = (struct WorkbenchBase *) OpenLibrary( WORKBENCHNAME, 37L )) ) {
        D(bug( "WBHandler: Could not open workbench.library!\n" ));
        goto exit;
    }

    /* Allocate a message port. */
    if( !(port = CreateMsgPort()) ) {
        D(bug( "WBHandler: Could not create message port!\n" ));
        goto exit;
    }

    /* Tell Workbench library we're here... */
    WorkbenchBase->wb_HandlerPort = port;

    /* Main event loop */
    do {
        WaitPort( port );

        while( (incoming = (struct WBStartup *) GetMsg( port )) != NULL ) {
            if( incoming->sm_Message.mn_Node.ln_Type == NT_REPLYMSG ) {
                /* This is a replied message from a WB Program that has finished */
                /* Deallocate it and the locks ands strings in WBArgs */

                openCount--;
            } else { /* Start a program */
                /* Allocate mem for message */
                /* Copy strings and duplicate locks */
                /* Load program and start it */
                /* Send message */

                openCount++;

                ReplyMsg( (struct Message *) incoming );
            }

        }
    } while( openCount > 0 );

exit:
    WorkbenchBase->wb_HandlerPort = NULL;

    if( port ) {
        DeleteMsgPort( port );
    }

    if( WorkbenchBase ) {
        CloseLibrary( (struct Library *) WorkbenchBase );
    }
}

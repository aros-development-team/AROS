/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>
#include <libcore/base.h>

#include <intuition/classes.h>
#include <intuition/classusr.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
 
#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

#include "preferenceswindow.h"
#include "preferenceswindow_methods.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Variables **************************************************************/
struct ExecBase        *SysBase;
struct Library         *MUIMasterBase;

struct MUI_CustomClass *MCC;


/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER( IPTR, PreferencesWindow_Dispatcher, CLASS, self, message )
{
    switch( message->MethodID )
    {
        case OM_NEW: return PreferencesWindow$OM_NEW( CLASS, self, (struct opSet *) message );
        default:     return DoSuperMethodA( CLASS, self, message );
    }
    
    return NULL;
}


/*** Library startup and shutdown *******************************************/
AROS_SET_LIBFUNC( MCC_Startup, LIBBASETYPE, LIBBASE )
{
    SysBase = LIBBASE->lh_SysBase;
    
    MUIMasterBase = OpenLibrary( "muimaster.library", 0 );
    if( MUIMasterBase == NULL ) return FALSE;
    
    MCC = MUI_CreateCustomClass( LIBBASE, "Window.mui", NULL, 0, PreferencesWindow_Dispatcher );
    if( MCC == NULL ) return FALSE;
    
    return TRUE;
}

AROS_SET_LIBFUNC( MCC_Shutdown, LIBBASETYPE, LIBBASE )
{
    CloseLibrary( MUIMasterBase );

    MUI_DeleteCustomClass( MCC );
}

ADD2INITLIB( MCC_Startup, 0 );
ADD2EXPUNGELIB( MCC_Shutdown, 0 );

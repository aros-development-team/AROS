/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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

#include <utility/date.h>
#include <libraries/mui.h>
#include <zune/clock.h>


/*** Variables **************************************************************/
struct Device      *TimerBase;
struct timerequest *TimerIO;
struct LocaleBase  *LocaleBase;

/*** Library startup and shutdown *******************************************/
AROS_SET_LIBFUNC( Calendar_Startup, LIBBASETYPE, LIBBASE )
{
    SysBase = LIBBASE->lh_SysBase;
    
    TimerIO    = NULL;
    TimerBase  = NULL;
    LocaleBase = NULL;
    
    TimerIO = AllocMem( sizeof( struct timerequest ), MEMF_CLEAR );
    if( TimerIO == NULL ) goto error;
    
    TimerIO->tr_node.io_Message.mn_Length = sizeof( struct timerequest );
    
    if( OpenDevice( "timer.device", UNIT_VBLANK, (struct IORequest *) TimerIO, 0) == 0 )
        TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
    else
        goto error;
        
    LocaleBase = (struct LocaleBase *) OpenLibrary( "locale.library", 0 );
    if( LocaleBase == NULL ) goto error;
        
    return TRUE;

error:
    if( LocaleBase != NULL ) CloseLibrary( (struct Library *) LocaleBase );
    if( TimerBase != NULL ) CloseDevice( TimerIO );
    if( TimerIO != NULL ) FreeMem( TimerIO, sizeof( struct timerequest ) );
    
    return FALSE;
}

AROS_SET_LIBFUNC( Calendar_Shutdown, LIBBASETYPE, LIBBASE )
{
    if( LocaleBase != NULL ) CloseLibrary( (struct Library *) LocaleBase );
    if( TimerBase != NULL ) CloseDevice( TimerIO );
    if( TimerIO != NULL ) FreeMem( TimerIO, sizeof( struct timerequest ) );
}

ADD2INITLIB( Calendar_Startup, 1 );
ADD2EXPUNGELIB( Calendar_Shutdown, 1 );

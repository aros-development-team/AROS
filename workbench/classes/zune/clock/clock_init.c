/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
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

struct MsgPort     *TimerMP;
struct timerequest *TimerIO;

/*** Library startup and shutdown *******************************************/
AROS_SET_LIBFUNC( Clock_Startup, LIBBASETYPE, LIBBASE )
{
    SysBase = LIBBASE->lh_SysBase;
    
    TimerMP   = NULL;
    TimerIO   = NULL;
    TimerBase = NULL;
    
    TimerMP = CreateMsgPort();
    if( TimerMP == NULL ) goto error;
    
    TimerIO = (struct timerequest *) CreateIORequest( TimerMP, sizeof(struct timerequest) );
    if( TimerIO == NULL ) goto error;
    
    if( OpenDevice( "timer.device", UNIT_VBLANK, (struct IORequest *) TimerIO, 0) == 0 )
        TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
    else
        goto error;
        
    return TRUE;

error:
    if( TimerBase != NULL ) CloseDevice( TimerBase );
    if( TimerIO != NULL ) DeleteIORequest( TimerIO );
    if( TimerMP != NULL ) DeleteMsgPort( TimerMP );

    return FALSE;
}

AROS_SET_LIBFUNC( Clock_Shutdown, LIBBASETYPE, LIBBASE )
{
    if( TimerBase != NULL ) CloseDevice( TimerBase );
    if( TimerIO != NULL ) DeleteIORequest( TimerIO );
    if( TimerMP != NULL ) DeleteMsgPort( TimerMP );
}

ADD2INITLIB( Clock_Startup, 1 );
ADD2EXPUNGELIB( Clock_Shutdown, 1 );

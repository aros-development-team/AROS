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

#include <proto/exec.h>
#include <proto/timer.h>

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

#include <utility/date.h>


/*** Variables **************************************************************/
struct Device      *TimerBase;
struct timerequest *TimerIO;

/*** Library startup and shutdown *******************************************/
AROS_SET_LIBFUNC(Clock_Startup, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    TimerIO   = NULL;
    TimerBase = NULL;
    
    TimerIO = AllocMem(sizeof(struct timerequest), MEMF_CLEAR);
    if(TimerIO == NULL) goto error;
    
    TimerIO->tr_node.io_Message.mn_Length = sizeof(struct timerequest);
    
    if
    (
        0 == OpenDevice
        (
            "timer.device", UNIT_VBLANK, (struct IORequest *) TimerIO, 0
        )
    )
    {
        TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
    }
    else
    {
        goto error;
    }
    
    return TRUE;

error:
    if (TimerBase != NULL) CloseDevice((struct IORequest *) TimerIO);
    if (TimerIO != NULL) FreeMem(TimerIO, sizeof(struct timerequest));
    
    return FALSE;

    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(Clock_Shutdown, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    if (TimerBase != NULL) CloseDevice((struct IORequest *) TimerIO);
    if (TimerIO != NULL) FreeMem(TimerIO, sizeof( struct timerequest ));

    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(Clock_Startup, 1);
ADD2EXPUNGELIB(Clock_Shutdown, 1);

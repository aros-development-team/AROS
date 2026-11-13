/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/timer.h>

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

#include <utility/date.h>


/*** Variables **************************************************************/
struct Device      *TimerBase;
struct timerequest *TimerIO;

/*** Library startup and shutdown *******************************************/
/*
 * Allocate a timerequest and open timer.device UNIT_VBLANK.
 * Return TRUE on success or release allocations and return FALSE.
 */
static int Clock_Startup(LIBBASETYPEPTR LIBBASE)
{
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
}

/*
 * Close timer.device and free request. Not idempotent: globals are
 * not cleared. Requires no active time readers; returns TRUE.
 */
static int Clock_Shutdown(LIBBASETYPEPTR LIBBASE)
{
    if (TimerBase != NULL) CloseDevice((struct IORequest *) TimerIO);
    if (TimerIO != NULL) FreeMem(TimerIO, sizeof( struct timerequest ));
    return TRUE;
}

ADD2INITLIB(Clock_Startup, 1);
ADD2EXPUNGELIB(Clock_Shutdown, 1);

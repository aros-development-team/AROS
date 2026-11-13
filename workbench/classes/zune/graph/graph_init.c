/*
    Copyright (C) 2017-2026, The AROS Development Team. All rights reserved.
    $Id: $
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
/* Initialize globals, allocate a cleared timerequest, and open timer.device
 * UNIT_VBLANK for library startup. Return TRUE on success; on failure close
 * any opened device and free the request before returning FALSE. Freed
 * globals are not reset afterward; this is not an idempotent retry/cleanup
 * contract.
 */
static int Graph_StartTimer(LIBBASETYPEPTR LIBBASE)
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

/* Close the timer device when TimerBase is non-NULL and free TimerIO when
 * non-NULL, then return TRUE. Intended for expunge after successful
 * startup. Globals are not reset, so repeated calls or cleanup after a
 * failed startup are not independently safe contracts.
 */
static int Graph_StopTimer(LIBBASETYPEPTR LIBBASE)
{
    if (TimerBase != NULL) CloseDevice((struct IORequest *) TimerIO);
    if (TimerIO != NULL) FreeMem(TimerIO, sizeof( struct timerequest ));
    return TRUE;
}

ADD2INITLIB(Graph_StartTimer, 1);
ADD2EXPUNGELIB(Graph_StopTimer, 1);

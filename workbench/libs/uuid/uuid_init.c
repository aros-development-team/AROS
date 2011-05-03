/*
    Copyright © 2007-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <dos/var.h>

#include "uuid_private.h"
#include LC_LIBDEFS_FILE

static int UUID_Init(LIBBASETYPEPTR LIBBASE)
{
    struct timeval tv;
    uuid_time_t time_now;
    
    /* Set up global lock (class and interface locks are separate!) */
    InitSemaphore(&LIBBASE->uuid_GlobalLock);
    
    D(bug("[UUID] UUID Init\n"));
    
    LIBBASE->uuid_Initialized = 0;
    
    /* I need timer.device in order to obtain system time */
    if (OpenDevice("timer.device", UNIT_MICROHZ, &LIBBASE->uuid_TR.tr_node, 0))
    {
        D(bug("[UUID] Could not open timer.device. ABORT!\n"));
        return FALSE;
    }
    
    /* get the system time and convert it to UUID time */
    GetSysTime(&tv);
    time_now = LIBBASE->uuid_NextUpdate = LIBBASE->uuid_LastTime = 
        ((uint64_t)tv.tv_secs + 2922) * 10000000 +
        ((uint64_t)tv.tv_micro) * 10 +
        ((uint64_t)0x01B21DD213814000LL);
    
    D(bug("[UUID] UUID time: 0x%08lx%08lx\n", 
          (uint32_t)((LIBBASE->uuid_LastTime >> 32) & 0xffffffff),
          (uint32_t)((LIBBASE->uuid_LastTime & 0xffffffff))
          ));
    
    /* Seed the random generator */
    time_now /= UUIDS_PER_TICK;
    UUIDBase->uuid_RandomSeed = (time_now >> 32) ^ time_now; 
    UUIDBase->uuid_UUIDs_ThisTick = 0;
    
    /* Try to open dos.library for GetVar/SetVar */
    DOSBase = (void *)OpenLibrary("dos.library", 0);
    if (DOSBase)
    {
        D(bug("[UUID] dos.library opened. Trying to get the UUID state.\n"));
        
        /* DOS is there. Try to get the last UUID state. */
        if (GetVar("uuid_state", (UBYTE*)&LIBBASE->uuid_State, sizeof(uuid_state_t),
               GVF_BINARY_VAR | GVF_DONT_NULL_TERM) == sizeof(uuid_state_t))
        {
            D(bug("[UUID] got last UUID state\n"));
            LIBBASE->uuid_Initialized = 1;
        }
        else
        {
            D(bug("[UUID] no UUID state found. Staying uninitlaized\n"));
        }
    }
    else
        D(bug("[UUID] dos.library not yet available. I will try later."));
    
    return TRUE;
}

static int UUID_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[UUID] Expunge. Byebye.\n"));
    
    CloseDevice(&LIBBASE->uuid_TR.tr_node);
    
    if (DOSBase)
    {
        /* Store the state */
        SetVar("uuid_state", (UBYTE*)&LIBBASE->uuid_State, sizeof(uuid_state_t),
               GVF_GLOBAL_ONLY | GVF_SAVE_VAR | LV_VAR);
        
        CloseLibrary((void *)DOSBase);
    }
    
    return TRUE;
}

ADD2INITLIB(UUID_Init, 0)
ADD2EXPUNGELIB(UUID_Expunge, 0)

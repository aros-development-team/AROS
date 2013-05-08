/*
    Copyright © 2007-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/uuid.h>
#include <proto/timer.h>
#include <proto/dos.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "uuid_private.h"
#include LC_LIBDEFS_FILE

static void uuid_generate_random(uuid_t *uuid, struct uuid_base *UUIDBase);
static void uuid_generate_time(uuid_t *uuid, struct uuid_base *UUIDBase);

/*****************************************************************************

    NAME */
        AROS_LH2(void, UUID_Generate,

/*  SYNOPSIS */                
        AROS_LHA(uuid_type_t, type, D0),
        AROS_LHA(uuid_t *, uuid, A0),
        
/*  LOCATION */
        struct uuid_base *, UUIDBase, 13, UUID)
        
/*  FUNCTION
        Generate Universally Unique Identifier conforming the RFC 4122.

    INPUTS
        type - type of the identifier:
               UUID_TYPE_DCE_RANDOM - random identifier. Do not use it on purpose
                                      due to the weak source of noise on AROS.
               UUID_TYPE_DCE_TIME - system time based identifier.
                                         
        uuid - storage for generated UUID.

    RESULT
        This function always succeeds.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT(uuid);
    
    ObtainSemaphore(&LIBBASE->uuid_GlobalLock);
    
    if (type == UUID_TYPE_DCE_RANDOM)
        uuid_generate_random(uuid, UUIDBase);
    else
        uuid_generate_time(uuid, UUIDBase);
    
    ReleaseSemaphore(&LIBBASE->uuid_GlobalLock);

    D({
        char buf[UUID_STRLEN+1];
        buf[UUID_STRLEN] = 0;
              
        UUID_Unparse(uuid, buf);
        bug("[UUID] generated UUID = %s\n", buf);
    })
    
    AROS_LIBFUNC_EXIT
}


static uint32_t uuid_rand(struct uuid_base *UUIDBase)
{
    return (UUIDBase->uuid_RandomSeed = 
        UUIDBase->uuid_RandomSeed * 1103515245 + 12345) % 0x7fffffff;
}

static void uuid_generate_random(uuid_t *uuid, struct uuid_base *UUIDBase)
{
    uint8_t u[16];
    int i;

    D(bug("[UUID] Generating random UUID\n"));
    
    for (i=0; i < 16; i++)
        u[i] = uuid_rand(UUIDBase);
    
    UUID_Unpack(u, uuid);
    
    uuid->clock_seq_hi_and_reserved &= 0x3f;
    uuid->clock_seq_hi_and_reserved |= 0x80;
    uuid->time_hi_and_version &= 0x0fff;
    uuid->time_hi_and_version |= 0x4000;
}

static void uuid_get_current_time(uuid_time_t *time, struct uuid_base *UUIDBase)
{
    uuid_time_t time_now;
    struct timeval tv;
    
    for (;;)
    {
        GetSysTime(&tv);
        time_now = ((uint64_t)tv.tv_secs + 2922) * 10000000 +
                   ((uint64_t)tv.tv_micro) * 10 +
                   ((uint64_t)0x01B21DD213814000LL);

        if (time_now != LIBBASE->uuid_LastTime)
        {
            UUIDBase->uuid_UUIDs_ThisTick = 0;
            LIBBASE->uuid_LastTime = time_now;
            break;
        }
        
        if (UUIDBase->uuid_UUIDs_ThisTick < UUIDS_PER_TICK)
        {
            UUIDBase->uuid_UUIDs_ThisTick++;
            break;
        }
    }
    *time = time_now + UUIDBase->uuid_UUIDs_ThisTick;
}

static void uuid_get_node(uuid_node_t *node, struct uuid_base *UUIDBase)
{
    if (!UUIDBase->uuid_Initialized)
    {
        if (!DOSBase)
            DOSBase = (void *)OpenLibrary("dos.library", 0);
        
        if (!(DOSBase && GetVar("uuid_state", (UBYTE*)&LIBBASE->uuid_State, sizeof(uuid_state_t),
                              GVF_BINARY_VAR | GVF_DONT_NULL_TERM) == sizeof(uuid_state_t)))
        {
            int i;
            UUIDBase->uuid_State.ts = UUIDBase->uuid_LastTime;
            UUIDBase->uuid_State.cs = uuid_rand(UUIDBase);
            for (i=0; i < 6; i++)
            {
                UUIDBase->uuid_State.node.nodeID[i] = uuid_rand(UUIDBase);                
            }
            UUIDBase->uuid_State.node.nodeID[0] |= 0x01;
        }
        LIBBASE->uuid_Initialized = 1;
        
        if (DOSBase)
            SetVar("uuid_state", (UBYTE*)&LIBBASE->uuid_State, sizeof(uuid_state_t),
                              GVF_BINARY_VAR | GVF_DONT_NULL_TERM | GVF_SAVE_VAR);
    }
    *node = UUIDBase->uuid_State.node;
}

static void uuid_get_state(uint16_t *cs, uuid_time_t *timestamp, uuid_node_t *node, struct uuid_base *UUIDBase)
{
    if (!UUIDBase->uuid_Initialized)
    {
        if (!DOSBase)
            DOSBase = (void *)OpenLibrary("dos.library", 0);

        if (!(DOSBase && GetVar("uuid_state", (UBYTE*)&LIBBASE->uuid_State, sizeof(uuid_state_t),
                                GVF_BINARY_VAR | GVF_DONT_NULL_TERM) == sizeof(uuid_state_t)))
        {
            int i;
            UUIDBase->uuid_State.ts = UUIDBase->uuid_LastTime;
            UUIDBase->uuid_State.cs = uuid_rand(UUIDBase);
            for (i=0; i < 6; i++)
            {
                UUIDBase->uuid_State.node.nodeID[i] = uuid_rand(UUIDBase);                
            }
            UUIDBase->uuid_State.node.nodeID[0] |= 0x01;
        }
        UUIDBase->uuid_Initialized = 1;
    }
    
    *node = UUIDBase->uuid_State.node;
    *timestamp = UUIDBase->uuid_State.ts;
    *cs = UUIDBase->uuid_State.cs;
}

static void uuid_set_state(uint16_t cs, uuid_time_t timestamp, uuid_node_t node, struct uuid_base *UUIDBase)
{
    UUIDBase->uuid_State.node = node;
    UUIDBase->uuid_State.ts = timestamp;
    UUIDBase->uuid_State.cs = cs;

    D(bug("[UUID] uuid_set_state(). Timestamp=%08x%08x, NextUpdate=%08x%08x\n",
          (uint32_t)(timestamp >> 32), (uint32_t)timestamp,
          (uint32_t)(LIBBASE->uuid_NextUpdate >> 32), (uint32_t)LIBBASE->uuid_NextUpdate
          ));
    
    if (timestamp >= LIBBASE->uuid_NextUpdate)
    {
        D(bug("[UUID] updating nonvolatile variable\n"));
        
        if (!DOSBase)
            DOSBase = (void *)OpenLibrary("dos.library", 0);
        
        if (DOSBase)
            SetVar("uuid_state", (UBYTE*)&LIBBASE->uuid_State, sizeof(uuid_state_t),
                              GVF_GLOBAL_ONLY | GVF_SAVE_VAR | LV_VAR);
        
        LIBBASE->uuid_NextUpdate = timestamp + (10 * 10 * 1000000);
    }    
}

static void uuid_generate_time(uuid_t *uuid, struct uuid_base *UUIDBase)
{
    uuid_time_t time, last_time;
    uuid_node_t node, last_node;
    uint16_t clockseq;
    
    D(bug("[UUID] Generating time-based UUID\n"));
    
    uuid_get_current_time(&time, UUIDBase);
    uuid_get_node(&node, UUIDBase);
    uuid_get_state(&clockseq, &last_time, &last_node, UUIDBase);
    
    if (memcmp(&node, &last_node, sizeof(node)))
        clockseq = uuid_rand(UUIDBase);
    else if (time < last_time)
        clockseq++;
    
    uuid_set_state(clockseq, time, node, UUIDBase);
    
    uuid->time_low = (uint32_t)(time & 0xFFFFFFFF);
    uuid->time_mid = (uint16_t)((time >> 32) & 0xFFFF);
    uuid->time_hi_and_version = (uint16_t)((time >> 48) & 0x0FFF); 
    uuid->time_hi_and_version |= (1 << 12); 
    uuid->clock_seq_low = clockseq & 0xFF;
    uuid->clock_seq_hi_and_reserved = (clockseq & 0x3F00) >> 8;
    uuid->clock_seq_hi_and_reserved |= 0x80;
    memcpy(&uuid->node, &node, sizeof uuid->node);
}


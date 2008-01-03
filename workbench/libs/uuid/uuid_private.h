#ifndef UUID_PRIVATE_H_
#define UUID_PRIVATE_H_

/*
    Copyright © 2007-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <devices/timer.h>
#include <dos/dos.h>

#include <libraries/uuid.h>

typedef struct {
    uuid_time_t  ts;       /* saved timestamp */
    uuid_node_t  node;     /* saved node ID */
    uint16_t     cs;       /* saved clock sequence */
} uuid_state_t;

struct uuid_base {
    struct Library              uuid_LibNode;
    struct SignalSemaphore      uuid_GlobalLock;
    struct DOSBase              *uuid_DOSBase;
    struct timerequest          uuid_TR;
    uuid_state_t                uuid_State;
    uuid_time_t                 uuid_NextUpdate;
    uuid_time_t                 uuid_LastTime;
    uint32_t                    uuid_RandomSeed;
    uint16_t                    uuid_UUIDs_ThisTick;
    uint8_t                     uuid_Initialized;
    
};
 
#ifdef DOSBase
#undef DOSBase
#endif
#define DOSBase (LIBBASE->uuid_DOSBase)

#ifdef TimerBase
#undef TimerBase
#endif
#define TimerBase (LIBBASE->uuid_TR.tr_node.io_Device)

#define UUIDS_PER_TICK 1024

#endif /*UUID_PRIVATE_H_*/

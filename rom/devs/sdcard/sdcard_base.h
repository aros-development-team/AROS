#ifndef _SDCARD_BASE_H
#define _SDCARD_BASE_H
/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>

#include "sdcard_bus.h"

// #define xxx(a) (a) to enable particular sections.
#if defined(DEBUG) && (DEBUG > 0)
#define DIRQ(a)
#define DIRQ_MORE(a)
#define DUMP(a)         
#define DUMP_MORE(a)
#define DINIT(a)        a
#define DTRANS(a)
#define DDEV(a)
#else
#define DIRQ(a)
#define DIRQ_MORE(a)
#define DUMP(a)
#define DUMP_MORE(a)
#define DINIT(a)
#define DTRANS(a)
#define DDEV(a)
#endif
/* Errors that shouldn't happen */
#define DERROR(a)       a

#define FNAME_SDC(x)                    SDCARD__Device__ ## x
#define FNAME_SDCIO(x)                  SDCARD__SDIO__ ## x

#define SDCARD_BUSTASKPRI               10
#define SDCARD_BUSTASKSTACK             16384

#define V200_MAXCLKDIV                  256
#define V300_MAXCLKDIV                  2046

#define SDCARD_TAGBASE                  (TAG_USER | 0x00534400)

#define SDCARD_TAG_CMD                  (SDCARD_TAGBASE + 1)
#define SDCARD_TAG_ARG                  (SDCARD_TAGBASE + 2)
#define SDCARD_TAG_RSPTYPE              (SDCARD_TAGBASE + 3)
#define SDCARD_TAG_RSP                  (SDCARD_TAGBASE + 4)
#define SDCARD_TAG_DATA                 (SDCARD_TAGBASE + 5)
#define SDCARD_TAG_DATALEN              (SDCARD_TAGBASE + 6)
#define SDCARD_TAG_DATAFLAGS            (SDCARD_TAGBASE + 7)

#define LED_OFF                         0
#define LED_ON                          100

/* sdhc.device base */
struct SDCardBase
{
    /* standard device structure */
    struct Device                       sdcard_Device;

    /* Imports */
    APTR		                sdcard_KernelBase;
    struct Device                       *sdcard_TimerBase;

    /* Bus's (to be replaced with hidds...) */
    struct SignalSemaphore              sdcard_BusSem;
    struct List                         sdcard_Buses;
    ULONG                               sdcard_BusCnt;
    ULONG                               sdcard_TotalBusUnits;
//    struct sdcard_Bus                   *sdcard_Bus;

   /* Memory Management */
   APTR                                 sdcard_MemPool;
};

#undef KernelBase
#define KernelBase SDCardBase->sdcard_KernelBase

#define IOStdReq(io)                    ((struct IOStdReq *)io)

/* Internal flags */

#define AB_Quirk_AtomicTMAndCMD         30
#define AB_Quirk_MissingCapabilities    29
#define AF_Quirk_AtomicTMAndCMD         (1 << AB_Quirk_AtomicTMAndCMD)
#define AF_Quirk_MissingCapabilities    (1 << AB_Quirk_MissingCapabilities)

#define SDCARD_BUSINITPRIO              10

BOOL FNAME_SDC(RegisterBus)(struct sdcard_Bus *, struct SDCardBase *);
BOOL FNAME_SDC(HandleIO)(struct IORequest *io);

/* IO Ops */
BYTE FNAME_SDCIO(ReadSector32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(ReadSector64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(ReadMultiple32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(ReadMultiple64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(ReadDMA32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(ReadDMA64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(WriteSector32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(WriteSector64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(WriteMultiple32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(WriteMultiple64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(WriteDMA32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(WriteDMA64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
BYTE FNAME_SDCIO(Eject)(struct sdcard_Unit *);

#endif /* _SDCARD_BASE_H */

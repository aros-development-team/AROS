#ifndef _SDCARD_BUS_H
#define _SDCARD_BUS_H
/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/ports.h>
#include <exec/io.h>

#define FNAME_SDCBUS(x)                 SDCARD__SDBus__ ## x

struct sdcard_BusUnits
{
      /* Unit Management */
    ULONG                               sdcbu_UnitBase;
    ULONG                               sdcbu_UnitMax;
    ULONG                               sdcbu_UnitCnt;
    struct sdcard_Unit                  *sdcbu_Units;      /* Units on the bus ( x sdcbu_UnitMax) */
};

struct sdcard_Bus
{
    struct Node                         sdcb_Node;
    struct SDCardBase                   *sdcb_DeviceBase;    /* Device self */

    ULONG                               sdcb_Quirks;

    APTR                                sdcb_IOBase;
    ULONG                               sdcb_BusIRQ;
    ULONG                               sdcb_BusNum;

    ULONG                               sdcb_BusFlags;       /* Bus flags similar to unit flags */
    ULONG                               sdcb_BusStatus;      /* copy of the status register */
    UBYTE                               sdcb_TaskSig;        /* Signal used to wake task */
    UBYTE                               sdcb_SectorShift;    /* Sector shift. 9 here is 512 bytes sector */

    struct Task                         *sdcb_Task;
    struct MsgPort                      *sdcb_MsgPort;
    struct IORequest                    *sdcb_Timer;         /* timer stuff */

    APTR                                sdcb_IRQHandle;

    /* Chipset .. */
    ULONG                               sdcb_Capabilities;
    ULONG                               sdcb_Version;
    ULONG                               sdcb_ClockMax;
    ULONG                               sdcb_ClockMin;
    ULONG                               sdcb_Power;          /* Supported Voltages */

    ULONG                               sdcb_IntrMask;

    /* Unit Management */
    struct sdcard_BusUnits              *sdcb_BusUnits;         /* Units on the bus */

    /* */
    BYTE                                (*sdcb_LEDCtrl)(int);
    UBYTE                               (*sdcb_IOReadByte)(ULONG, struct sdcard_Bus *);
    UWORD                               (*sdcb_IOReadWord)(ULONG, struct sdcard_Bus *);
    ULONG                               (*sdcb_IOReadLong)(ULONG, struct sdcard_Bus *);

    void                                (*sdcb_IOWriteByte)(ULONG, UBYTE, struct sdcard_Bus *);
    void                                (*sdcb_IOWriteWord)(ULONG, UWORD, struct sdcard_Bus *);
    void                                (*sdcb_IOWriteLong)(ULONG, ULONG, struct sdcard_Bus *);

    /* Bus Instance Private/Internal */
    IPTR                                sdcb_Private;
};

/* Bus Flags .. */
#define AB_Bus_MediaPresent             30     /* media available */
#define AB_Bus_MediaChanged             29     /* media changed */
#define AB_Bus_SPI                      28

#define AF_Bus_MediaPresent             (1 << AB_Bus_MediaPresent)
#define AF_Bus_MediaChanged             (1 << AB_Bus_MediaChanged)
#define AF_Bus_SPI                      (1 << AB_Bus_SPI)

BOOL FNAME_SDCBUS(RegisterUnit)(struct sdcard_Bus *);
BOOL FNAME_SDCBUS(StartUnit)(struct sdcard_Unit *);

void FNAME_SDCBUS(SoftReset)(UBYTE, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(GetClockDiv)(ULONG, struct sdcard_Bus *);
void FNAME_SDCBUS(SetClock)(ULONG, struct sdcard_Bus *);
void FNAME_SDCBUS(SetPowerLevel)(ULONG, BOOL, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(SendCmd)(struct TagItem *, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(WaitCmd)(ULONG, ULONG, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(FinishCmd)(struct TagItem *, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(FinishData)(struct TagItem *, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(Rsp136Unpack)(ULONG *, ULONG, const ULONG);

void FNAME_SDCBUS(BusIRQ)(struct sdcard_Bus *, struct TagItem *);
void FNAME_SDCBUS(BusTask)(struct sdcard_Bus *);

#endif /* _SDCARD_BUS_H */

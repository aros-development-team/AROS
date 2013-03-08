#ifndef _SDCARD_INTERN_H
#define _SDCARD_INTERN_H
/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <utility/utility.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <devices/newstyle.h>
#include <devices/timer.h>
#include <devices/cd.h>

#define FNAME_SDC(x)                    SDCARD__Device__ ## x
#define FNAME_SDCIO(x)                  SDCARD__SDIO__ ## x
#define FNAME_SDCBUS(x)                 SDCARD__SDBus__ ## x

#define TASK_PRI		        10
#define TIMEOUT			        30

#define HOSTCLOCK_MIN                   400000
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

/* structure forward declarations */
struct sdcard_Unit;
struct sdcard_Bus;

/* sdhc.device base */
struct SDCardBase
{
    /* standard device structure */
    struct Device                       sdcard_Device;

    /* Imports */
    struct Device                       *sdcard_TimerBase;
    APTR                                sdcard_VCMBoxBase;

    /* Bus's (to be replaced with hidds...) */
    struct sdcard_Bus                   *sdcard_Bus;

   /* Memory Management */
   APTR                                 sdcard_MemPool;
};

struct sdcard_Bus
{
    struct SDCardBase                   *sdcb_DeviceBase;    /* Device self */

    APTR                                sdcb_IOBase;
    ULONG                               sdcb_BusNum;

    UBYTE                               sdcb_BusFlags;       /* Bus flags similar to unit flags */
    UBYTE                               sdcb_TaskSig;        /* Signal used to wake task */
    UBYTE                               sdcb_SectorShift;    /* Sector shift. 9 here is 512 bytes sector */

    struct Task                         *sdcb_Task;
    struct MsgPort                      *sdcb_MsgPort;
    struct IORequest                    *sdcb_Timer;         /* timer stuff */

    /* Chipset .. */
    ULONG                               sdcb_Capabilities;
    ULONG                               sdcb_Version;
    ULONG                               sdcb_ClockMax;
    ULONG                               sdcb_Power;

    ULONG                               sdcb_IntrMask;

    ULONG                               sdcb_LastWrite;
    
    ULONG                               sdcb_UnitCnt;
    struct sdcard_Unit                  *sdcb_Units[1];      /* Units on the bus */
};

/*
   Unit structure describing given device on the bus. It contains all the
   necessary information unit/device may need.
   */
struct sdcard_Unit
{
    struct Unit                         sdcu_Unit;             /* exec's unit */
    struct sdcard_Bus                   *sdcu_Bus;             /* Bus on which this unit is */
    ULONG                               sdcu_Flags;            /* Unit flags, see below */
    ULONG                               sdcu_UnitNum;          /* Unit number as coded by device */
    ULONG                               sdcu_CardRCA;

    ULONG                               sdcu_CardPower;        /* voltages supported by card/controller */

    UQUAD                               sdcu_Capacity;         /* Real capacity of device */

/* OLD DEFINES */
    
   ULONG               sdcu_DMAPort;

   ULONG               sdcu_Cylinders;
   UBYTE               sdcu_Heads;
   UBYTE               sdcu_Sectors;
   UBYTE               sdcu_Model[41];
   UBYTE               sdcu_FirmwareRev[9];
   UBYTE               sdcu_SerialNumber[21];

   /*
      Here are stored pointers to functions responsible for handling this
      device. They are set during device initialization and point to most
      effective functions for this particular unit. Read/Write may be done
      in PIO mode reading single sectors, using multisector PIO, or
      multiword DMA.
      */
   BYTE        (*sdcu_Read32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
   BYTE        (*sdcu_Write32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
   BYTE        (*sdcu_Read64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
   BYTE        (*sdcu_Write64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
   BYTE        (*sdcu_Eject)(struct sdcard_Unit *);
   BYTE        (*sdcu_DirectSCSI)(struct sdcard_Unit *, struct SCSICmd*);

   ULONG               sdcu_ChangeNum;   /* Number of disc changes */

   struct Interrupt   *sdcu_RemoveInt;  /* Raise this interrupt on a disc change */
   struct List         sdcu_SoftList;    /* Raise even more interrupts from this list on disc change */

   UBYTE               sdcu_DevMask;             /* device mask used to simplify device number coding */
   UBYTE               sdcu_SenseKey;            /* Sense key from ATAPI devices */
   UBYTE               sdcu_DevType;

   /******* PIO IO ********/
   APTR                sdcu_cmd_data;
   ULONG               sdcu_cmd_length;
   ULONG               sdcu_cmd_total;
   ULONG               sdcu_cmd_error;
};

/* Unit internal flags */
#define AB_MediaPresent                 30     /* media available */
#define AB_MediaChanged                 29     /* media changed */
#define AB_HighSpeed                    28
#define AB_HighCapacity                 27
#define AB_MMC                          26
#define AB_4bitData                     25

#define AF_MediaPresent                 (1 << AB_MediaPresent)
#define AF_MediaChanged                 (1 << AB_MediaChanged)
#define AF_HighSpeed                    (1 << AB_HighSpeed)
#define AF_HighCapacity                 (1 << AB_HighCapacity)
#define AF_MMC                          (1 << AB_MMC)
#define AF_4bitData                     (1 << AB_4bitData)

#define Unit(io)                        ((struct sdcard_Unit *)(io)->io_Unit)
#define IOStdReq(io)                    ((struct IOStdReq *)io)

UBYTE FNAME_SDCBUS(MMIOReadByte)(ULONG, struct sdcard_Bus *);
UWORD FNAME_SDCBUS(MMIOReadWord)(ULONG, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(MMIOReadLong)(ULONG, struct sdcard_Bus *);

void FNAME_SDCBUS(MMIOWriteByte)(ULONG, UBYTE, struct sdcard_Bus *);
void FNAME_SDCBUS(MMIOWriteWord)(ULONG, UWORD, struct sdcard_Bus *);
void FNAME_SDCBUS(MMIOWriteLong)(ULONG, ULONG, struct sdcard_Bus *);

void FNAME_SDCBUS(SoftReset)(UBYTE, struct sdcard_Bus *);
void FNAME_SDCBUS(SetClock)(ULONG, struct sdcard_Bus *);
void FNAME_SDCBUS(SetPowerLevel)(ULONG, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(SendCmd)(struct TagItem *, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(Rsp136Unpack)(ULONG *, ULONG, const ULONG);

void FNAME_SDCBUS(BusTask)(struct sdcard_Bus *);

BOOL FNAME_SDC(RegisterVolume)(struct sdcard_Bus *);

int FNAME_SDCBUS(SDUnitChangeFrequency)(struct sdcard_Unit *);

BOOL FNAME_SDC(HandleIO)(struct IORequest *io);

/* IO Operations */

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

#endif // _SDCARD_INTERN_H


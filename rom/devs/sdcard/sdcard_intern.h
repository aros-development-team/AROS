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

#include <asm/bcm2835.h>

#define FNAME_SDC(x)                    SDCARD__Device__ ## x
#define FNAME_SDCIO(x)                  SDCARD__SDIO__ ## x
#define FNAME_SDCBUS(x)                 SDCARD__SDBus__ ## x

// #define xxx(a) (a) to enable particular sections.
#if defined(DEBUG) && (DEBUG > 0)
#define DIRQ(a)         a
#define DIRQ_MORE(a)
#define DUMP(a)         a
#define DUMP_MORE(a)
#define DINIT(a)        a
#else
#define DIRQ(a)
#define DIRQ_MORE(a)
#define DUMP(a)
#define DUMP_MORE(a)
#define DINIT(a)
#endif
/* Errors that shouldn't happen */
#define DERROR(a) a

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

#define LED_OFF                         0
#define LED_ON                          100

static inline void sdcard_SetLED(int lvl)
{
    if (lvl > 0)
        *(volatile ULONG *)GPCLR0 = (1 << 16); // Turn Activity LED ON
    else
        *(volatile ULONG *)GPSET0 = (1 << 16); // Turn Activity LED OFF
}

/* structure forward declarations */
struct sdcard_Unit;
struct sdcard_Bus;

/* sdhc.device base */
struct SDCardBase
{
    /* standard device structure */
    struct Device                       sdcard_Device;

    /* Imports */
    APTR		                sdcard_KernelBase;
    struct Device                       *sdcard_TimerBase;
    APTR                                sdcard_VCMBoxBase;

    /* Bus's (to be replaced with hidds...) */
    struct sdcard_Bus                   *sdcard_Bus;

   /* Memory Management */
   APTR                                 sdcard_MemPool;
};

#undef KernelBase
#define KernelBase SDCardBase->sdcard_KernelBase

struct sdcard_Bus
{
    struct SDCardBase                   *sdcb_DeviceBase;    /* Device self */

    APTR                                sdcb_IOBase;
    ULONG                               sdcb_BusNum;

    ULONG                               sdcb_BusFlags;       /* Bus flags similar to unit flags */
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
    ULONG                               sdcb_Power;          /* Supported Voltages */

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

/* Internal flags */
#define AB_Bus_MediaPresent             30     /* media available */
#define AB_Bus_MediaChanged             29     /* media changed */
#define AB_Bus_SPI                      28

#define AB_Card_HighSpeed52             30
#define AB_Card_HighSpeed               29
#define AB_Card_HighCapacity            28
#define AB_Card_MMC                     27
#define AB_Card_4bitData                26

#define AF_Bus_MediaPresent             (1 << AB_Bus_MediaPresent)
#define AF_Bus_MediaChanged             (1 << AB_Bus_MediaChanged)
#define AF_Bus_SPI                      (1 << AB_Bus_SPI)

#define AF_Card_HighSpeed52             (1 << AB_Card_HighSpeed52)
#define AF_Card_HighSpeed               (1 << AB_Card_HighSpeed)
#define AF_Card_HighCapacity            (1 << AB_Card_HighCapacity)
#define AF_Card_4bitData                (1 << AB_Card_4bitData)
#define AF_Card_MMC                     (1 << AB_Card_MMC)

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
void FNAME_SDCBUS(SetPowerLevel)(ULONG, BOOL, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(SendCmd)(struct TagItem *, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(WaitCmd)(ULONG, ULONG, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(FinishCmd)(struct TagItem *, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(FinishData)(struct TagItem *, struct sdcard_Bus *);
ULONG FNAME_SDCBUS(Rsp136Unpack)(ULONG *, ULONG, const ULONG);

void FNAME_SDCBUS(BusIRQ)(struct sdcard_Bus *, struct TagItem *);
void FNAME_SDCBUS(BusTask)(struct sdcard_Bus *);

BOOL FNAME_SDC(RegisterVolume)(struct sdcard_Bus *);

ULONG FNAME_SDCBUS(WaitUnitStatus)(ULONG, struct sdcard_Unit *);
ULONG FNAME_SDCBUS(SDSCChangeFrequency)(struct sdcard_Unit *);
ULONG FNAME_SDCBUS(MMCChangeFrequency)(struct sdcard_Unit *);

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


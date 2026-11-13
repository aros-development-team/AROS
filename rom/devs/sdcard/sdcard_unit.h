#ifndef _SDCARD_UNIT_H
#define _SDCARD_UNIT_H
/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <devices/scsidisk.h>

#define FNAME_SDCUNIT(x)                SDCARD__SDUnit__ ## x

struct sdcard_Bus;

/*
   Unit structure describing given device on the bus. It contains all the
   necessary information unit/device may need.
   */
struct sdcard_Unit
{
    struct Unit                         sdcu_Unit;             /* exec's unit */
    struct sdcard_Bus                   *sdcu_Bus;             /* Bus to which this unit is attached */
    ULONG                               sdcu_Flags;            /* Unit flags, see below */
    ULONG                               sdcu_UnitNum;          /* Unit number as coded by device */
    ULONG                               sdcu_CardRCA;

    ULONG                               sdcu_CardPower;        /* voltages supported by card/controller */

    UQUAD                               sdcu_Capacity;         /* Real capacity of device */
    ULONG                               sdcu_Cylinders;
    UBYTE                               sdcu_Heads;
    UBYTE                               sdcu_Sectors;

   /*
      Here are stored pointers to functions responsible for handling this
      device. They are set during device initialization and point to most
      effective functions for this particular unit. Read/Write may be done
      in PIO mode reading single sectors, using multisector PIO, or
      multiword DMA.
      */
   BYTE                                 (*sdcu_Read32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
   BYTE                                 (*sdcu_Write32)(struct sdcard_Unit *, ULONG, ULONG, APTR, ULONG *);
   BYTE                                 (*sdcu_Read64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
   BYTE                                 (*sdcu_Write64)(struct sdcard_Unit *, UQUAD, ULONG, APTR, ULONG *);
   BYTE                                 (*sdcu_DirectSCSI)(struct sdcard_Unit *, struct SCSICmd *);
   BYTE                                 (*sdcu_Eject)(struct sdcard_Unit *);

   ULONG                                sdcu_ChangeNum;   /* Number of disc changes */

   struct Interrupt                     *sdcu_RemoveInt;  /* Raise this interrupt on a disc change */
   struct List                          sdcu_SoftList;    /* Raise even more interrupts from this list on disc change */

   UBYTE                                sdcu_DevMask;             /* device mask used to simplify device number coding */
   UBYTE                                sdcu_SenseKey;            /* Sense key from ATAPI devices */
   UBYTE                                sdcu_DevType;
};

#define Unit(io)                        ((struct sdcard_Unit *)(io)->io_Unit)

/* Unit Flags .. */

#define AB_Card_Active                  30
#define AB_Card_HighSpeed52             27
#define AB_Card_HighSpeed               26
#define AB_Card_HighCapacity            25
#define AB_Card_4bitData                20
#define AB_Card_MMC                     16
#define AB_Card_WriteProtect            1
#define AB_Card_Locked                  0

#define AF_Card_Active                  (1 << AB_Card_Active)
#define AF_Card_HighSpeed52             (1 << AB_Card_HighSpeed52)
#define AF_Card_HighSpeed               (1 << AB_Card_HighSpeed)
#define AF_Card_HighCapacity            (1 << AB_Card_HighCapacity)
#define AF_Card_4bitData                (1 << AB_Card_4bitData)
#define AF_Card_MMC                     (1 << AB_Card_MMC)
#define AF_Card_WriteProtect            (1 << AB_Card_WriteProtect)
#define AF_Card_Locked                  (1 << AB_Card_Locked)

ULONG FNAME_SDCUNIT(WaitStatus)(ULONG, struct sdcard_Unit *);
ULONG FNAME_SDCUNIT(SDSCChangeFrequency)(struct sdcard_Unit *);
ULONG FNAME_SDCUNIT(MMCChangeFrequency)(struct sdcard_Unit *);

#endif /* _SDCARD_UNIT_H */

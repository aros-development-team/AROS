#ifndef _ATA_H
#define _ATA_H

/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ata.device main private include file
    Lang: English
*/

#define __OOP_NOMETHODBASES__

#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <oop/oop.h>
#include <utility/hooks.h>
#include <utility/utility.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <devices/newstyle.h>
#include <devices/timer.h>
#include <devices/cd.h>
#include <hardware/ata.h>
#include <hidd/ata.h>

#include "include/devices/scsicmds.h"

#define MAX_DEVICEBUSES         2
#define MAX_BUSUNITS            2
#define STACK_SIZE              16384
#define TASK_PRI                10
#define TIMEOUT                 30

/*
   Don't blame me for information redundance here!

   Please note, that all structures here are more or less chained together.
   The aim is, every single function in ata.device, no matter whether it takes
   ata_Unit or ata_Bus or God knows what else, would have access to ata device
   base and through it, to all other device structures.

   I just wanted to avoid passing ataBase everywhere. :-D
   */

/* structure forward declarations */
struct ata_Unit;
struct ata_Bus;

/* ata.device base */
struct ataBase
{
   struct Device           ata_Device;    /* Exec device structure                */
   struct Task            *ata_Daemon;    /* master task pointer                  */
   struct MsgPort         *DaemonPort;    /* Daemon's message port                */
   struct MinList          Daemon_ios;    /* Daemon's IORequests                  */
   struct SignalSemaphore  DaemonSem;
   struct Task            *daemonParent;  /* Who sends control requests to daemon */
   int                     ata__buscount; /* Number of all buses                  */
   struct SignalSemaphore  DetectionSem;  /* Device detection semaphore           */

   /* Arguments and flags */
   UBYTE                   ata_32bit;
   UBYTE                   ata_NoMulti;
   UBYTE                   ata_NoDMA;
   UBYTE                   ata_Poll;

   /*
    * memory pool
    */
   APTR                    ata_MemPool;

   ULONG                   ata_ItersPer100ns;

   struct Library         *ata_OOPBase;
   struct Library         *ata_UtilityBase;
   BPTR                    ata_SegList;

   /* Bus HIDD classes */
   OOP_AttrBase            unitAttrBase;
   OOP_AttrBase            hwAttrBase;
   OOP_AttrBase            ataAttrBase;
   OOP_MethodID            hwMethodBase;
   OOP_MethodID            ataMethodBase;
   OOP_Class              *ataClass;
   OOP_Class              *busClass;
   OOP_Class              *unitClass;
   OOP_Object             *ataObj;
};

#undef HWAttrBase
#undef HiddATABusAB
#undef HiddATAUnitAB
#undef HWBase
#undef HiddATABusBase
#define HWAttrBase     (ATABase->hwAttrBase)
#define HiddATABusAB   (ATABase->ataAttrBase)
#define HiddATAUnitAB  (ATABase->unitAttrBase)
#define HWBase         (ATABase->hwMethodBase)
#define HiddATABusBase (ATABase->ataMethodBase)
#define OOPBase        (ATABase->ata_OOPBase)
#define UtilityBase    (ATABase->ata_UtilityBase)

/*
   The single IDE bus (channel)
   */
struct ata_Bus
{
   struct ataBase          *ab_Base;   /* device self */
   /* Bus object data */
   struct ATA_BusInterface *busVectors;     /* Control vector table     */
   struct ATA_PIOInterface *pioVectors;     /* PIO vector table         */
   APTR                    *dmaVectors;     /* DMA vector table         */
   ULONG                   pioDataSize;     /* PIO interface data size  */
   ULONG                   dmaDataSize;     /* DMA interface data size  */
   void                    *pioInterface;   /* PIO interface object     */
   void                    *dmaInterface;   /* DMA interface object     */
   BOOL                    keepEmpty;       /* Whether we should keep empty bus object */

   UBYTE                   ab_Dev[2];  /* Master/Slave type, see below */
   UBYTE                   ab_Flags;   /* Bus flags similar to unit flags */
   BYTE                    ab_SleepySignal; /* Signal used to wake the task up, when it's waiting */
   /* for data requests/DMA */
   UBYTE                   ab_BusNum;  /* bus id - used to calculate device id */
   volatile LONG           ab_Timeout; /* in seconds; please note that resolution is low (1sec) */

   struct ata_Unit         *ab_Units[MAX_BUSUNITS];    /* Units on the bus */
   struct ata_Unit         *ab_SelectedUnit;    /* Currently selected unit */

   ULONG                   ab_IntCnt;

   struct Task             *ab_Task;       /* Bus task handling all not-immediate transactions */
   struct MsgPort          *ab_MsgPort;    /* Task's message port */
   struct IORequest        *ab_Timer;      /* timer stuff */

   struct Interrupt        ab_ResetInt;

   /* functions go here */
   void                   (*ab_HandleIRQ)(struct ata_Unit* unit, UBYTE status);
};

/* Device types */
#define DEV_NONE        0x00
#define DEV_UNKNOWN     0x01
#define DEV_ATA         0x02
#define DEV_SATA        0x03
#define DEV_ATAPI       0x80
#define DEV_SATAPI      0x81

/*
   DriveIdent structure as returned by ATA_IDENTIFY_[DEVICE|ATAPI]
   */
struct DriveIdent {
   UWORD       id_General;             // 0
   UWORD       id_OldCylinders;        // 1
   UWORD       id_SpecificConfig;      // 2
   UWORD       id_OldHeads;            // 3
   UWORD       pad1[2];                // 4-5
   UWORD       id_OldSectors;          // 6
   UWORD       pad2[3];                // 7-9
   UBYTE       id_SerialNumber[20];    // 10-19
   UWORD       pad3[3];                // 20-22
   UBYTE       id_FirmwareRev[8];      // 23-26
   UBYTE       id_Model[40];           // 27-46
   UWORD       id_RWMultipleSize;      // 47
   UWORD       pad4;                   // 48
   UWORD       id_Capabilities;        // 49
   UWORD       id_OldCaps;             // 50
   UWORD       id_OldPIO;              // 51
   UWORD       pad5;                   // 52
   UWORD       id_ConfigAvailable;     // 53
   UWORD       id_OldLCylinders;       // 54
   UWORD       id_OldLHeads;           // 55
   UWORD       id_OldLSectors;         // 56
   UWORD       pad6[2];                // 57-58
   UWORD       id_RWMultipleTrans;     // 59
   ULONG       id_LBASectors;          // 60-61
   UWORD       id_DMADir;              // 62
   UWORD       id_MWDMASupport;        // 63
   UWORD       id_PIOSupport;          // 64
   UWORD       id_MWDMA_MinCycleTime;  // 65
   UWORD       id_MWDMA_DefCycleTime;  // 66
   UWORD       id_PIO_MinCycleTime;    // 67
   UWORD       id_PIO_MinCycleTimeIORDY; // 68
   UWORD       pad8[6];                // 69-74
   UWORD       id_QueueDepth;          // 75
   UWORD       pad9[4];                // 76-79
   UWORD       id_ATAVersion;          // 80
   UWORD       id_ATARevision;         // 81
   UWORD       id_Commands1;           // 82
   UWORD       id_Commands2;           // 83
   UWORD       id_Commands3;           // 84
   UWORD       id_Commands4;           // 85
   UWORD       id_Commands5;           // 86
   UWORD       id_Commands6;           // 87
   UWORD       id_UDMASupport;         // 88
   UWORD       id_SecurityEraseTime;   // 89
   UWORD       id_ESecurityEraseTime;  // 90
   UWORD       id_CurrentAdvPowerMode; // 91
   UWORD       id_MasterPwdRevision;   // 92
   UWORD       id_HWResetResult;       // 93
   UWORD       id_AcousticManagement;  // 94
   UWORD       id_StreamMinimunReqSize; // 95
   UWORD       id_StreamingTimeDMA;    // 96
   UWORD       id_StreamingLatency;    // 97
   ULONG       id_StreamingGranularity; // 98-99
   UQUAD       id_LBA48Sectors;        // 100-103
   UWORD       id_StreamingTimePIO;    // 104
   UWORD       pad10;                  // 105
   UWORD       id_PhysSectorSize;      // 106
   UWORD       pad11;                  // 107
   UQUAD       id_UniqueIDi[2];        // 108-115
   UWORD       pad12;                  // 116
   ULONG       id_WordsPerLogicalSector; // 117-118
   UWORD       pad13[8];               // 119-126
   UWORD       id_RemMediaStatusNotificationFeatures; // 127
   UWORD       id_SecurityStatus;      // 128
   UWORD       pad14[127];
} __attribute__((packed));

typedef struct
{
   UBYTE command;       // current ATA command
   UBYTE feature;       // FF to indicate no feature
   UBYTE secmul;        // for read multiple - multiplier. default 1
   UBYTE pad;
   UQUAD blk;
   ULONG sectors;
   APTR  buffer;
   ULONG length;
   ULONG actual;

   enum
   {
      CM_NoData,
      CM_PIORead,
      CM_PIOWrite,
      CM_DMARead,
      CM_DMAWrite
   } method;
   enum
   {
      CT_NoBlock,
      CT_CHS,
      CT_LBA28,
      CT_LBA48,
   } type;
} ata_CommandBlock;

struct DaemonIO
{
    struct MinNode link;
    struct IOStdReq req;
};

/*
   Unit structure describing given device on the bus. It contains all the
   necessary information unit/device may need.
   */
struct ata_Unit
{
   struct Unit         au_Unit;        /* exec's unit */
   struct DriveIdent  *au_Drive;       /* Drive Ident after IDENTIFY command */
   struct ata_Bus     *au_Bus;         /* Bus on which this unit is */
   struct IOStdReq    *DaemonReq;      /* Disk change monitoring request */

   ULONG               au_XferModes;   /* available transfer modes */

   ULONG               au_Capacity;    /* Highest sector accessible through LBA28 */
   UQUAD               au_Capacity48;  /* Highest sector accessible through LBA48 */
   ULONG               au_Cylinders;
   UBYTE               au_Heads;
   UBYTE               au_Sectors;
   UBYTE               au_Model[41];
   UBYTE               au_FirmwareRev[9];
   UBYTE               au_SerialNumber[21];

   /*
      Here are stored pointers to functions responsible for handling this
      device. They are set during device initialization and point to most
      effective functions for this particular unit. Read/Write may be done
      in PIO mode reading single sectors, using multisector PIO, or
      multiword DMA.
      */
   BYTE                (*au_Read32    )(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
   BYTE                (*au_Write32   )(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
   BYTE                (*au_Read64    )(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
   BYTE                (*au_Write64   )(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
   BYTE                (*au_Eject     )(struct ata_Unit *);
   BYTE                (*au_DirectSCSI)(struct ata_Unit *, struct SCSICmd*);
   BYTE                (*au_Identify  )(struct ata_Unit *);
   VOID                (*au_ins       )(APTR, APTR, ULONG);
   VOID                (*au_outs      )(APTR, APTR, ULONG);
   void                *pioInterface;  /* PIO interface object, cached for performance */

   ULONG               au_UnitNum;     /* Unit number as coded by device */
   ULONG               au_Flags;       /* Unit flags, see below */
   ULONG               au_ChangeNum;   /* Number of disc changes */

   struct Interrupt   *au_RemoveInt;   /* Raise this interrupt on a disc change */
   struct List         au_SoftList;    /* Raise even more interrupts from this list on disc change */

   UBYTE               au_SectorShift; /* Sector shift. 9 here is 512 bytes sector */
   UBYTE               au_DevMask;     /* device mask used to simplify device number coding */
   UBYTE               au_SenseKey;    /* Sense key from ATAPI devices */
   UBYTE               au_DevType;

   /******* PIO IO ********/
   APTR                au_cmd_data;
   ULONG               au_cmd_length;
   ULONG               au_cmd_total;
   ULONG               au_cmd_error;
};

/* Unit internal flags */
#define AB_DiscPresent          30     /* disc now in drive */
#define AB_DiscChanged          29     /* disc changed */
#define AB_Removable            28     /* media removable */
#define AB_CHSOnly              26     /* only supports CHS commands */

#define AF_DiscPresent          (1 << AB_DiscPresent)
#define AF_DiscChanged          (1 << AB_DiscChanged)
#define AF_Removable            (1 << AB_Removable)
#define AF_CHSOnly              (1 << AB_CHSOnly)

#define Unit(io) ((struct ata_Unit *)(io)->io_Unit)
#define IOStdReq(io) ((struct IOStdReq *)io)

/* Function prototypes */

BOOL Hidd_ATABus_Start(OOP_Object *o, struct ataBase *ATABase);
AROS_UFP3(BOOL, Hidd_ATABus_Open,
          AROS_UFPA(struct Hook *, h, A0),
          AROS_UFPA(OOP_Object *, obj, A2),
          AROS_UFPA(IPTR, reqUnit, A1));
AROS_UFP3(BOOL, Hidd_ATABus_Tick,
          AROS_UFPA(struct Hook *, h, A0),
          AROS_UFPA(OOP_Object *, obj, A2),
          AROS_UFPA(struct ataBase *, ATABase, A1));

void ata_ResetBus(struct ata_Bus *);
void ata_InitBus(struct ata_Bus *);

BYTE atapi_SendPacket(struct ata_Unit *, APTR, APTR, LONG, BOOL*, BOOL);
int atapi_TestUnitOK(struct ata_Unit *);

BYTE atapi_Identify(struct ata_Unit*);
BYTE ata_Identify(struct ata_Unit*);

BYTE atapi_DirectSCSI(struct ata_Unit*, struct SCSICmd *);
ULONG atapi_RequestSense(struct ata_Unit* unit, UBYTE* sense, ULONG senselen);

BOOL ata_setup_unit(struct ata_Bus *bus, struct ata_Unit *unit);
void ata_init_unit(struct ata_Bus *bus, struct ata_Unit *unit, UBYTE u);
BOOL ata_RegisterVolume(ULONG StartCyl, ULONG EndCyl, struct ata_Unit *unit);

void BusTaskCode(struct ata_Bus *bus, struct ataBase *ATABase);
void DaemonCode(struct ataBase *LIBBASE);

#define ATAPI_SS_EJECT  0x02
#define ATAPI_SS_LOAD   0x03

struct atapi_StartStop
{
    UBYTE   command;
    UBYTE   immediate;
    UBYTE   pad1[2];
    UBYTE   flags;
    UBYTE   pad2[7];
};

BYTE SCSIEmu(struct ata_Unit*, struct SCSICmd*);

#endif // _ATA_H


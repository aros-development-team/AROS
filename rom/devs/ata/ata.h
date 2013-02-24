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
#include "ata_bus.h"

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

/*
 * this **might** cause problems with PPC64, which **might** expect both to be 64bit.
 */
struct PRDEntry {
   ULONG   prde_Address;
   ULONG   prde_Length;
};

#define PRDE_EOT    0x80000000
#define PRD_MAX     514

/* ata.device base */
struct ataBase
{
   /*
    * Device structure - used to manage devices by exec guts^h^h^hoods
    */
   struct Device           ata_Device;

   /*
    * master task pointer
    */
   struct Task            *ata_Daemon;

   /*
    * list of all buses - we may have more than just 4
    */
   struct MinList          ata_Buses;
   int                     ata__buscount;

   /*
    * Arguments and flags
    */
   UBYTE                   ata_32bit;
   UBYTE                   ata_NoMulti;
   UBYTE                   ata_NoDMA;
   UBYTE                   ata_Poll;
   STRPTR                  ata_CmdLine;

   /*
    * memory pool
    */
   APTR                    ata_MemPool;

   struct Device          *ata_TimerBase;
   ULONG                   ata_ItersPer100ns;

   struct Library         *ata_OOPBase;

   /* Bus HIDD classes */
   OOP_AttrBase            hwAttrBase;
   OOP_AttrBase            ataAttrBase;
   OOP_MethodID            hwMethodBase;
   OOP_MethodID            ataMethodBase;
   OOP_Class              *ataClass;
   OOP_Class              *busClass;
   OOP_Object             *ataObj;
};

#undef HWAttrBase
#undef HiddATABusAB
#undef HWBase
#undef HiddATABusBase
#define HWAttrBase     (ATABase->hwAttrBase)
#define HiddATABusAB   (ATABase->ataAttrBase)
#define HWBase         (ATABase->hwMethodBase)
#define HiddATABusBase (ATABase->ataMethodBase)
#define OOPBase        (ATABase->ata_OOPBase)

/*
   The single IDE bus (channel)
   */
struct ata_Bus
{
   struct MinNode          ab_Node;    /* exec node */
   struct ataBase          *ab_Base;   /* device self */
   /* Bus object data */
   APTR                    *busVectors;     /* Control vector table     */
   struct ATA_PIOInterface *pioVectors;     /* PIO vector table         */
   APTR                    *dmaVectors;     /* DMA vector table         */
   ULONG                   pioDataSize;     /* PIO interface data size  */
   ULONG                   dmaDataSize;     /* DMA interface data size  */
   void                    *pioInterface;   /* PIO interface object     */
   void                    *dmaInterface;   /* DMA interface object     */

   ULONG                   ab_Port;    /* IO port used */
   ULONG                   ab_Alt;     /* alternate io port */
   UBYTE                   ab_IRQ;     /* IRQ number used */
   UBYTE                   ab_Dev[2];  /* Master/Slave type, see below */
   UBYTE                   ab_Flags;   /* Bus flags similar to unit flags */
   BYTE                    ab_SleepySignal; /* Signal used to wake the task up, when it's waiting */
   /* for data requests/DMA */
   UBYTE                   ab_BusNum;  /* bus id - used to calculate device id */
   volatile LONG           ab_Timeout; /* in seconds; please note that resolution is low (1sec) */

   struct ata_Unit         *ab_Units[MAX_BUSUNITS];    /* Units on the bus */
   struct ata_Unit         *ab_SelectedUnit;    /* Currently selected unit */

   struct Interrupt        ab_IntHandler;
   ULONG                   ab_IntCnt;

   struct Task             *ab_Task;       /* Bus task handling all not-immediate transactions */
   struct MsgPort          *ab_MsgPort;    /* Task's message port */
   struct PRDEntry         *ab_PRD;
   struct IORequest        *ab_Timer;      /* timer stuff */

   struct Interrupt        ab_ResetInt;

   /* functions go here */
   void                   (*ab_HandleIRQ)(struct ata_Unit* unit, UBYTE status);
   
   /* Bus driver stuff */
   const struct ata_BusDriver *ab_Driver;
   APTR                        ab_DriverData;
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

/*
   Unit structure describing given device on the bus. It contains all the
   necessary information unit/device may need.
   */
struct ata_Unit
{
   struct Unit         au_Unit;   /* exec's unit */
   struct DriveIdent  *au_Drive;  /* Drive Ident after IDENTIFY command */
   struct ata_Bus     *au_Bus;    /* Bus on which this unit is */

   ULONG               au_DMAPort;
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
   BYTE        (*au_Read32)(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
   BYTE        (*au_Write32)(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
   BYTE        (*au_Read64)(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
   BYTE        (*au_Write64)(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
   BYTE        (*au_Eject)(struct ata_Unit *);
   BYTE        (*au_DirectSCSI)(struct ata_Unit *, struct SCSICmd*);
   BYTE        (*au_Identify)(struct ata_Unit *);

   VOID                (*au_ins)(APTR, UWORD, ULONG, APTR);
   VOID                (*au_outs)(APTR, UWORD, ULONG, APTR);

   ULONG               au_UnitNum;     /* Unit number as coded by device */
   ULONG               au_Flags;       /* Unit flags, see below */
   ULONG               au_ChangeNum;   /* Number of disc changes */

   struct Interrupt   *au_RemoveInt;  /* Raise this interrupt on a disc change */
   struct List         au_SoftList;    /* Raise even more interrupts from this list on disc change */

   UBYTE               au_SectorShift;         /* Sector shift. 9 here is 512 bytes sector */
   UBYTE               au_DevMask;             /* device mask used to simplify device number coding */
   UBYTE               au_SenseKey;            /* Sense key from ATAPI devices */
   UBYTE               au_DevType;

   /******* PIO IO ********/
   APTR                au_cmd_data;
   ULONG               au_cmd_length;
   ULONG               au_cmd_total;
   ULONG               au_cmd_error;
};

enum
{
   AB_XFER_48BIT = AB_XFER_NUM,
   AB_XFER_RWMULTI,
   AB_XFER_PACKET,
   AB_XFER_LBA,
   AB_XFER_DMA,
};

#define AF_XFER_PIO(x)  (1<<(AB_XFER_PIO0+(x)))
#define AF_XFER_MDMA(x) (1<<(AB_XFER_MDMA0+(x)))
#define AF_XFER_UDMA(x) (1<<(AB_XFER_UDMA0+(x)))
#define AF_XFER_48BIT   (1<<(AB_XFER_48BIT))
#define AF_XFER_RWMULTI (1<<(AB_XFER_RWMULTI))
#define AF_XFER_PACKET  (1<<(AB_XFER_PACKET))
#define AF_XFER_LBA     (1<<(AB_XFER_LBA))
#define AF_XFER_DMA     (1<<(AB_XFER_DMA))

/* Unit internal flags */
#define AB_DiscPresent          30     /* disc now in drive */
#define AB_DiscChanged          29     /* disc changed */
#define AB_Removable            28     /* media removable */
#define AB_80Wire               27     /* has an 80-wire cable */
#define AB_CHSOnly              26     /* only supports CHS commands */

#define AF_DiscPresent          (1 << AB_DiscPresent)
#define AF_DiscChanged          (1 << AB_DiscChanged)
#define AF_Removable            (1 << AB_Removable)
#define AF_80Wire               (1 << AB_80Wire)
#define AF_CHSOnly              (1 << AB_CHSOnly)

/* RegisterBus flags */
#define ARBB_80Wire              0
#define ARBB_EarlyInterrupt      1

#define ARBF_80Wire              (1 << ARBB_80Wire)
#define ARBF_EarlyInterrupt      (1 << ARBB_EarlyInterrupt)

#define ATA_OUT(val, offset, port)  unit->au_Bus->ab_Driver->ata_out((val), (offset), (port), unit->au_Bus->ab_DriverData)
#define ATA_IN(offset, port)        unit->au_Bus->ab_Driver->ata_in((offset), (port), unit->au_Bus->ab_DriverData)
#define ATA_OUTL(val, offset, port) unit->au_Bus->ab_Driver->ata_outl((val), (offset), (port), unit->au_Bus->ab_DriverData)

#define BUS_OUT(val, offset, port)  bus->ab_Driver->ata_out((val), (offset), (port), bus->ab_DriverData)
#define BUS_IN(offset, port)        bus->ab_Driver->ata_in((offset), (port), bus->ab_DriverData)
#define BUS_OUTL(val, offset, port) bus->ab_Driver->ata_outl((val), (offset), (port), bus->ab_DriverData)

/* SFF-8038i DMA registers */
#define dma_Command         0x00
#define dma_Status          0x02
#define dma_PRD             0x04

/* DMA command register */
#define DMA_READ            0x00    /* PCI *READS* from memory to drive */
#define DMA_WRITE           0x08    /* PCI *WRITES* to memory from drive */
#define DMA_START           0x01    /* DMA Start/Stop */

#define DMAB_Active         0
#define DMAB_Error          1
#define DMAB_Interrupt      2
#define DMAB_Simplex        7

#define DMAF_Active         (1 << DMAB_Active)
#define DMAF_Error          (1 << DMAB_Error)
#define DMAF_Interrupt      (1 << DMAB_Interrupt)
#define DMAF_Simplex        (1 << DMAB_Simplex)

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

BOOL ata_HandleIRQ(struct ata_Bus *bus);

BOOL dma_SetupPRD(struct ata_Unit *, APTR, ULONG, BOOL);
BOOL dma_SetupPRDSize(struct ata_Unit *, APTR, ULONG, BOOL);
VOID dma_StartDMA(struct ata_Unit *);
VOID dma_StopDMA(struct ata_Unit *);
VOID dma_Cleanup(APTR adr, ULONG len, BOOL read);

BOOL ata_setup_unit(struct ata_Bus *bus, UBYTE u);
BOOL ata_init_unit(struct ata_Bus *bus, UBYTE u);
BOOL ata_RegisterVolume(ULONG StartCyl, ULONG EndCyl, struct ata_Unit *unit);

void ata_RegisterBus(IPTR IOBase, IPTR IOAlt, IPTR INTLine, IPTR DMABase, ULONG Flags,
                     const struct ata_BusDriver *driver, APTR driverData, struct ataBase *ATABase);

void BusTaskCode(struct ata_Bus *bus, struct Task* parent, struct SignalSemaphore *ssem);
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


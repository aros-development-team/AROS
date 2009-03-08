#ifndef _ATA_H
#define _ATA_H

/*
   Copyright © 2004-2008, The AROS Development Team. All rights reserved
   $Id$

Desc: ata.device main private include file
Lang: English
*/
/*
 * CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2008-01-25  T. Wiszkowski       Rebuilt, rearranged and partially fixed 60% of the code here
 *                                 Enabled implementation to scan for other PCI IDE controllers
 *                                 Implemented ATAPI Packet Support for both read and write
 *                                 Corrected ATAPI DMA handling
 *                                 Fixed major IDE enumeration bugs severely handicapping transfers with more than one controller
 *                                 Compacted source and implemented major ATA support procedure
 *                                 Improved DMA and Interrupt management
 *                                 Removed obsolete code
 * 2008-03-23  T. Wiszkowski       Corrected DMA PRD issue (x86_64 systems)
 * 2008-03-30  T. Wiszkowski       Added workaround for interrupt collision handling; fixed SATA in LEGACY mode.
 *                                 nForce and Intel SATA chipsets should now be operational.
 * 2008-03-31  M. Schulz           We do have asm/io.h include for ages... No need to define io functions here anymore.
 *                                 Redefined ata_in and ata_out. On x86-like systems they use inb/outb directly. On other systems
 *                                 they use pci_inb and pci_outb.
 * 2008-04-05  T. Wiszkowski       Improved IRQ management
 * 2008-04-07  T. Wiszkowski       Changed bus timeout mechanism
 * 2008-05-11  T. Wiszkowski       Remade the ata trannsfers altogether, corrected the pio/irq handling
 *                                 medium removal, device detection, bus management and much more
 * 2008-06-24  P. Fedin            Added 'nomulti' flag to disable multisector operations
 * 2009-02-21  M. Schulz           ata_in/ata_out declared as functions, if no PCI-io operations are defined.
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
#include <aros/bootloader.h>
#include "include/cd.h"
#include "include/scsicmds.h"

#include <hidd/irq.h>
#include <asm/io.h>

#define MAX_DEVICEBUSES		2
#define MAX_BUSUNITS		2
#define STACK_SIZE		16384
#define TASK_PRI		10

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

    struct List		ata__legacybuses;
    struct List		ata__probedbuses;
    int 		ata__buscount;
   /*
    * list of all buses - we may have more than just 4
    */
   struct MinList          ata_Buses;

   /*
    * flags
    */
   UBYTE                   ata_32bit;
   UBYTE                   ata_NoMulti;
   UBYTE                   ata_NoDMA;
   UBYTE                   ata_NoSubclass;
   UBYTE                   ata_ScanFlags;

#define ATA_SCANPCI		(1 << 0)
#define ATA_SCANLEGACY		(1 << 1)
   /*
    * memory pool
    */
   APTR                    ata_MemPool;
};

/*
   The single IDE bus (channel)
   */
struct ata_Bus
{
   struct MinNode          ab_Node;    /* exec node */
   struct ataBase          *ab_Base;   /* device self */
   ULONG                   ab_Port;    /* IO port used */
   ULONG                   ab_Alt;     /* alternate io port */
   UBYTE                   ab_Irq;     /* IRQ used */
   UBYTE                   ab_Dev[2];  /* Master/Slave type, see below */
   UBYTE                   ab_Flags;   /* Bus flags similar to unit flags */
   BYTE                    ab_SleepySignal; /* Signal used to wake the task up, when it's waiting */
   /* for data requests/DMA */
   UBYTE                   ab_BusNum;  /* bus id - used to calculate device id */
   BOOL                    ab_IRQ;     /* set if IRQ is enabled */
   LONG                    ab_Timeout; /* in seconds; please note that resolution is low (1sec) */

   struct ata_Unit         *ab_Units[MAX_BUSUNITS];    /* Units on the bus */

   HIDDT_IRQ_Handler       *ab_IntHandler;
   ULONG                   ab_IntCnt;

   struct Task             *ab_Task;       /* Bus task handling all not-immediate transactions */
   struct MsgPort          *ab_MsgPort;    /* Task's message port */
   struct PRDEntry         *ab_PRD;
   struct IORequest	   *ab_Timer;	   /* timer stuff */

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
   UWORD       pad7;                   // 62
   UWORD       id_MWDMASupport;        // 63
   UWORD       id_PIOSupport;          // 64
   UWORD       id_MWDMA_MinCycleTime;  // 65
   UWORD       id_MWDMA_DefCycleTime;  // 66
   UWORD       id_PIO_MinCycleTime;    // 67
   UWORD       id_PIO_MinCycleTImeIORDY; // 68
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
   UWORD       id_EnchSecurityEraseTime; // 90
   UWORD       id_CurrentAdvowerMode;  // 91
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
      in PIO mode reading single sectors, using PIO with multiword or DMA.
      */
   ULONG               (*au_Read32)(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
   ULONG               (*au_Write32)(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
   ULONG               (*au_Read64)(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
   ULONG               (*au_Write64)(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
   ULONG               (*au_Eject)(struct ata_Unit *);
   ULONG               (*au_DirectSCSI)(struct ata_Unit *, struct SCSICmd*);
   ULONG               (*au_Identify)(struct ata_Unit *);

   VOID                (*au_ins)(APTR, UWORD, ULONG);
   VOID                (*au_outs)(APTR, UWORD, ULONG);

   /* If a HW driver is used with this unit, it may store its data here */
   APTR                au_DriverData;

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

typedef enum
{
   AB_XFER_PIO0 = 0,
   AB_XFER_PIO1,
   AB_XFER_PIO2,
   AB_XFER_PIO3,
   AB_XFER_PIO4,
   AB_XFER_PIO5,
   AB_XFER_PIO6,
   AB_XFER_PIO7,

   AB_XFER_MDMA0,
   AB_XFER_MDMA1,
   AB_XFER_MDMA2,
   AB_XFER_MDMA3,
   AB_XFER_MDMA4,
   AB_XFER_MDMA5,
   AB_XFER_MDMA6,
   AB_XFER_MDMA7,

   AB_XFER_UDMA0,
   AB_XFER_UDMA1,
   AB_XFER_UDMA2,
   AB_XFER_UDMA3,
   AB_XFER_UDMA4,
   AB_XFER_UDMA5,
   AB_XFER_UDMA6,
   AB_XFER_UDMA7,

   AB_XFER_48BIT,
   AB_XFER_RWMULTI,
   AB_XFER_PACKET,
   AB_XFER_LBA,
   AB_XFER_DMA,

} ata_XferMode;

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

#define AF_DiscPresent          (1 << AB_DiscPresent)
#define AF_DiscChanged          (1 << AB_DiscChanged)
#define AF_Removable            (1 << AB_Removable)

/* ATA/ATAPI registers */
#define ata_Error           1
#define ata_Feature         1
#define ata_Count           2
#define ata_LBALow          3
#define ata_LBAMid          4
#define ata_LBAHigh         5
#define ata_DevHead         6
#define ata_Status          7
#define ata_Command         7
#define ata_AltStatus       0x2
#define ata_AltControl      0x2

#if defined(__i386__) || defined(__x86_64__)
#define ata_out(val, offset, port)  outb((val), (offset)+(port))
#define ata_in(offset, port)        inb((offset)+(port))
#define ata_outl(val, offset, port) outl((val), (offset)+(port))
#elif !defined (pci_outb)
void ata_out(UBYTE val, UWORD offset, IPTR port);
UBYTE ata_in(UWORD offset, IPTR port);
void ata_outl(ULONG val, UWORD offset, IPTR port);
#else
#define ata_out(val, offset, port)  pci_outb((val), (offset)+(port))
#define ata_in(offset, port)        pci_inb((offset)+(port))
#define ata_outl(val, offset, port) pci_outl_le((val), (offset)+(port))
#endif


#define atapi_Error         1
#define atapi_Features      1
#define atapi_Reason        2
#define atapi_ByteCntL      4
#define atapi_ByteCntH      5
#define atapi_DevSel        6
#define atapi_Status        7
#define atapi_Command       7

/* Atapi status bits */
#define ATAB_SLAVE          4
#define ATAB_LBA            6
#define ATAB_ATAPI          7
#define ATAB_DATAREQ        3
#define ATAB_ERROR          0
#define ATAB_BUSY           7

#define ATAF_SLAVE          0x10
#define ATAF_LBA            0x40
#define ATAF_ATAPI          0x80
#define ATAF_DATAREQ        0x08
#define ATAF_ERROR          0x01
#define ATAF_BUSY           0x80
#define ATAF_DRDY           0x40

#define ATAPIF_CHECK        0x01

/* ATA/ATAPI commands */
#define ATA_SET_FEATURES    0xef
#define ATA_SET_MULTIPLE    0xc6
#define ATA_DEVICE_RESET    0x08
#define ATA_IDENTIFY_DEVICE 0xec
#define ATA_IDENTIFY_ATAPI  0xa1
#define ATA_NOP             0x00
#define ATA_EXECUTE_DIAG    0x90
#define ATA_PACKET          0xa0
#define ATA_READ_DMA        0xc8
#define ATA_READ_DMA64      0x25
#define ATA_READ            0x20
#define ATA_READ64          0x24
#define ATA_READ_MULTIPLE   0xc4
#define ATA_READ_MULTIPLE64 0x29
#define ATA_WRITE_DMA       0xca
#define ATA_WRITE_DMA64     0x35
#define ATA_WRITE           0x30
#define ATA_WRITE64         0x34
#define ATA_WRITE_MULTIPLE  0xc5
#define ATA_WRITE_MULTIPLE64 0x39
#define ATA_MEDIA_EJECT     0xed

#define ATAPIF_MASK         0x03
#define ATAPIF_COMMAND      0x01
#define ATAPIF_READ         0x02
#define ATAPIF_WRITE        0x00

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

void ata_ResetBus(struct ata_Bus *);
void ata_InitBus(struct ata_Bus *);

int atapi_SendPacket(struct ata_Unit *, APTR, APTR, LONG, BOOL*, BOOL);
int atapi_TestUnitOK(struct ata_Unit *);

ULONG atapi_Identify(struct ata_Unit*);
ULONG ata_Identify(struct ata_Unit*);

ULONG atapi_DirectSCSI(struct ata_Unit*, struct SCSICmd *);
ULONG atapi_RequestSense(struct ata_Unit* unit, UBYTE* sense, ULONG senselen);
void ata_EnableIRQ(struct ata_Bus *bus, BOOL enable);

int ata_InitBusTask(struct ata_Bus *, struct SignalSemaphore*);
int ata_InitDaemonTask(struct ataBase *);
void ata_HandleIRQ(struct ata_Bus *bus);
UBYTE ata_ReadStatus(struct ata_Bus *bus);

BOOL dma_SetupPRD(struct ata_Unit *, APTR, ULONG, BOOL);
BOOL dma_SetupPRDSize(struct ata_Unit *, APTR, ULONG, BOOL);
VOID dma_StartDMA(struct ata_Unit *);
VOID dma_StopDMA(struct ata_Unit *);
VOID dma_Cleanup(APTR adr, ULONG len, BOOL read);

BOOL ata_setup_unit(struct ata_Bus *bus, UBYTE u);
BOOL ata_init_unit(struct ata_Bus *bus, UBYTE u);
BOOL ata_RegisterVolume(ULONG StartCyl, ULONG EndCyl, struct ata_Unit *unit);

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

#if 0
/*
    Arch specific things to access IO space of drive. Shouldn't be here. Really.
*/
static inline ULONG inl(UWORD port)
{
    ULONG val;
    asm volatile ("inl %w1,%0":"=a"(val):"Nd"(port));

    return val;
}

static inline UWORD inw(UWORD port)
{
    UWORD val;
    asm volatile ("inw %w1,%0":"=a"(val):"Nd"(port));

    return val;
}

static inline UBYTE inb(UWORD port)
{
    UBYTE val;
    asm volatile ("inb %w1,%0":"=a"(val):"Nd"(port));

    return val;
}

static inline VOID outl(ULONG val, UWORD port)
{
    asm volatile ("outl %0,%w1"::"a"(val),"Nd"(port));
}

static inline VOID outw(UWORD val, UWORD port)
{
    asm volatile ("outw %0,%w1"::"a"(val),"Nd"(port));
}

static inline VOID outb(UBYTE val, UWORD port)
{
    asm volatile ("outb %0,%w1"::"a"(val),"Nd"(port));
}
#endif

#endif // _ATA_H


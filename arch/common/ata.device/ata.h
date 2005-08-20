#ifndef _ATA_H
#define _ATA_H

/*
    Copyright © 2004, The AROS Development Team. All rights reserved
    $Id$

    Desc: ata.device main private include file
    Lang: English
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

#include LC_LIBDEFS_FILE

#define MAX_UNIT    2
#define MAX_BUS	    4
#define STACK_SIZE  16384
#define TASK_PRI    10

extern UBYTE LIBEND;

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

struct PRDEntry {
    IPTR    prde_Address;
    ULONG   prde_Length;
};

#define PRDE_EOT    0x8000
#define PRD_MAX	    514

/* ata.device base */
struct ataBase {
    struct Device	    ata_Device;
    struct ExecBase	    *ata_SysBase;
    struct UtilityBase	    *ata_UtilityBase;
    struct Library	    *ata_OOPBase;
    
    struct Task		    *ata_Daemon;
    struct ata_Bus	    *ata_Buses[MAX_BUS];

    UBYTE		    ata_32bit;
    UBYTE		    ata_ForceDMA;

    APTR		    ata_MemPool;
};

/*
    The single IDE bus (channel)
*/
struct ata_Bus {
    struct ataBase	    *ab_Base;	/* device self */
    struct SignalSemaphore  ab_Lock;	/* Semaphore locking IO access */
    ULONG		    ab_Port;	/* IO port used */
    UBYTE		    ab_Irq;	/* IRQ used */
    UBYTE		    ab_Dev[2];	/* Master/Slave type, see below */
    UBYTE		    ab_Flags;	/* Bus flags similar to unit flags */
    BYTE		    ab_SleepySignal; /* Signal used to wake the task up, when it's waiting */
					     /* for data requests/DMA */
    BOOL		    ab_Waiting;
    ULONG		    ab_Timeout;

    struct ata_Unit	    *ab_Units[MAX_UNIT];    /* Units on the bus */

    HIDDT_IRQ_Handler	    *ab_IntHandler;
    ULONG		    ab_IntCnt;

    struct Task		    *ab_Task;	    /* Bus task handling all not-immediate transactions */
    struct MsgPort	    *ab_MsgPort;    /* Task's message port */
    struct MsgPort	    *ab_TimerMP;    /* Two fields used by bus task to do delays if needed */
    struct timerequest	    *ab_TimerIO;
    struct PRDEntry	    *ab_PRD;
};

/* Device types */
#define DEV_NONE	0x00
#define	DEV_UNKNOWN	0x01
#define	DEV_ATA		0x02
#define DEV_ATAPI	0x80

/*
    DriveIdent structure as returned by ATA_IDENTIFY_[DEVICE|ATAPI]
*/
struct DriveIdent {
    UWORD	id_General;	// 0
    UWORD	id_OldCylinders;	// 1
    UWORD	id_SpecificConfig;  // 2
    UWORD	id_OldHeads;	// 3
    UWORD   pad1[2];	    // 4-5
    UWORD	id_OldSectors;	// 6
    UWORD   pad2[3];	    // 7-9
    UBYTE	id_SerialNumber[20];    // 10-19
    UWORD   pad3[3];	    // 20-22
    UBYTE	id_FirmwareRev[8];	// 23-26
    UBYTE	id_Model[40];	    // 27-46
    UWORD	id_RWMultipleSize;	// 47
    UWORD   pad4;		// 48
    UWORD	id_Capabilities;	// 49
    UWORD	id_OldCaps;	// 50
    UWORD	id_OldPIO;	    // 51
    UWORD   pad5;		// 52
    UWORD	id_ConfigAvailable;	// 53
    UWORD	id_OldLCylinders;   // 54
    UWORD	id_OldLHeads;	// 55
    UWORD	id_OldLSectors;	    // 56
    UWORD   pad6[2];		// 57-58
    UWORD	id_RWMultipleTrans; // 59
    ULONG	id_LBASectors;	// 60-61
    UWORD   pad7;		// 62
    UWORD	id_MWDMASupport;	// 63
    UWORD	id_PIOSupport;	    // 64
    UWORD	id_MWDMA_MinCycleTime;  // 65
    UWORD	id_MWDMA_DefCycleTime;	// 66
    UWORD	id_PIO_MinCycleTime;    // 67
    UWORD	id_PIO_MinCycleTImeIORDY; // 68
    UWORD   pad8[6];		// 69-74
    UWORD	id_QueueDepth;	    // 75
    UWORD   pad9[4];		// 76-79
    UWORD	id_ATAVersion;	    // 80
    UWORD	id_ATARevision;	// 81
    UWORD	id_Commands1;	    // 82
    UWORD	id_Commands2;	// 83
    UWORD	id_Commands3;	    // 84
    UWORD	id_Commands4;	// 85
    UWORD	id_Commands5;	    // 86
    UWORD	id_Commands6;	// 87
    UWORD	id_UDMASupport;	    // 88
    UWORD	id_SecurityEraseTime;   // 89
    UWORD	id_EnchSecurityEraseTime; // 90
    UWORD	id_CurrentAdvowerMode;  // 91
    UWORD	id_MasterPwdRevision;	// 92
    UWORD	id_HWResetResult;	// 93
    UWORD	id_AcousticManagement;	// 94
    UWORD	id_StreamMinimunReqSize; // 95
    UWORD	id_StreamingTimeDMA;	// 96
    UWORD	id_StreamingLatency;    // 97
    ULONG	id_StreamingGranularity; // 98-99
    UQUAD	id_LBA48Sectors;	// 100-103
    UWORD	id_StreamingTimePIO;	// 104
    UWORD   pad10;	    // 105
    UWORD	id_PhysSectorSize;  // 106
    UWORD   pad11;	    // 107
    UQUAD	id_UniqueIDi[2];    // 108-115
    UWORD   pad12;	    // 116
    ULONG	id_WordsPerLogicalSector; // 117-118
    UWORD   pad13[8];		// 119-126
    UWORD	id_RemMediaStatusNotificationFeatures; // 127
    UWORD	id_SecurityStatus;	// 128
    UWORD   pad14[127];
} __attribute__((packed));


/*
    Unit structure describing given device on the bus. It contains all the
    necessary information unit/device may need.
*/
struct ata_Unit {
    struct Unit		au_Unit;    /* exec's unit */
    struct ataBase	*au_Base;   /* device's base */
    struct DriveIdent	*au_Drive;  /* Drive Ident after IDENTIFY command */
    struct ata_Bus	*au_Bus;    /* Bus on which this unit is */

    ULONG		au_DMAPort;

    ULONG		au_Capacity;	/* Highest sector accessible through LBA28 */
    UQUAD		au_Capacity48;	/* Highest sector accessible through LBA48 */
    ULONG		au_Cylinders;
    UBYTE		au_Heads;
    UBYTE		au_Sectors;
    UBYTE		au_Model[41];
    UBYTE		au_FirmwareRev[9];
    UBYTE		au_SerialNumber[21];

    /*
	Here are stored pointers to functions responsible for handling this
        device. They are set during device initialization and point to most
	effective functions for this particular unit. Read/Write may be done
	in PIO mode reading single sectors, using PIO with multiword or DMA.
    */
    ULONG		(*au_Read32)(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
    ULONG		(*au_Write32)(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
    ULONG		(*au_Read64)(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
    ULONG		(*au_Write64)(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
    ULONG		(*au_Eject)(struct ata_Unit *);

    VOID		(*au_ins)(APTR, UWORD, ULONG);
    VOID		(*au_outs)(APTR, UWORD, ULONG);

    /* If a HW driver is used with this unit, it may store its data here */
    APTR		au_DriverData;
    
    ULONG		au_UnitNum;	/* Unit number as coded by device */
    ULONG		au_Flags;	/* Unit flags, see below */
    ULONG		au_ChangeNum;	/* Number of disc changes */
    ULONG		au_NumLoop;	/* Maximal busy wait delay for unit */

    struct Interrupt	*au_RemoveInt;	/* Raise this interrupt on a disc change */
    struct List		au_SoftList;	/* Raise even more interrupts from this list on disc change */

    UBYTE	au_RDBSector;		/* Geee. Do I really need it? */
    UBYTE	au_SectorShift;		/* Sector shift. 9 here is 512 bytes sector */
    UBYTE	au_DevMask;		/* device mask used to simplify device number coding */
    UBYTE	au_SenseKey;		/* Sense key from ATAPI devices */
    UBYTE	au_DevType;
};

/* Unit internal flags */
#define AB_ATAPI		31	
#define AB_DiscPresent		30
#define AB_DiscPresenceUnknown	29 
#define AB_Removable		28
#define AB_Used			27
#define AB_SlowDevice		26

#define AF_ATAPI		(1 << AB_ATAPI)
#define AF_DiscPresent		(1 << AB_DiscPresent)
#define AF_DiscPresenceUnknown	(1 << AB_DiscPresenceUnknown)
#define AF_Removable		(1 << AB_Removable)
#define AF_Used			(1 << AB_Used)
#define AF_SlowDevice		(1 << AB_SlowDevice)

/* ATA/ATAPI registers */
#define ata_Error	    1
#define ata_Count	    2
#define ata_LBALow	    3
#define ata_LBAMid	    4
#define ata_LBAHigh	    5
#define ata_DevHead	    6
#define ata_Status	    7
#define ata_Command	    7
#define ata_AltStatus	    0x206
#define ata_Control	    0x206

#define ata_out(val, offset, port)  outb((val), (offset)+(port))
#define ata_in(offset, port)	    inb((offset)+(port))

#define atapi_Error	    1
#define atapi_Features	    1
#define atapi_Reason	    2
#define atapi_ByteCntL	    4
#define atapi_ByteCntH	    5
#define atapi_DevSel	    6
#define atapi_Status	    7
#define atapi_Command	    7

/* Atapi status bits */
#define ATAB_SLAVE	    4
#define ATAB_LBA	    6
#define ATAB_ATAPI	    7
#define ATAB_DATAREQ	    3
#define ATAB_ERROR	    0
#define ATAB_BUSY	    7

#define ATAF_SLAVE	    0x10
#define ATAF_LBA	    0x40
#define ATAF_ATAPI	    0x80
#define ATAF_DATAREQ	    0x08
#define ATAF_ERROR	    0x01
#define ATAF_BUSY	    0x80

#define ATAPIF_CHECK	    0x01

/* ATA/ATAPI commands */
#define ATA_SET_FEATURES    0xef
#define ATA_SET_MULTIPLE    0xc6
#define ATA_DEVICE_RESET    0x08
#define ATA_IDENTIFY_DEVICE 0xec
#define ATA_IDENTIFY_ATAPI  0xa1
#define ATA_NOP		    0x00
#define ATA_PACKET	    0xa0
#define ATA_READ_DMA	    0xc8
#define ATA_READ_DMA64	    0x25
#define ATA_READ	    0x20
#define ATA_READ64	    0x24
#define ATA_READ_MULTIPLE   0xc4
#define ATA_READ_MULTIPLE64 0x29
#define ATA_WRITE_DMA	    0xca
#define ATA_WRITE_DMA64	    0x35
#define ATA_WRITE	    0x30
#define ATA_WRITE64	    0x34
#define ATA_WRITE_MULTIPLE  0xc5
#define ATA_WRITE_MULTIPLE64 0x39
#define ATA_MEDIA_EJECT	    0xed

#define ATAPIF_MASK	    0x03
#define ATAPIF_COMMAND	    0x01
#define ATAPIF_READ	    0x02
#define ATAPIF_WRITE	    0x00

/* SFF-8038i DMA registers */
#define dma_Command	    0x00
#define dma_Status	    0x02
#define dma_PRD		    0x04

/* DMA command register */
#define DMA_READ	    0x00    /* PCI *READS* from memory to drive */
#define DMA_WRITE	    0x08    /* PCI *WRITES* to memory from drive */
#define DMA_START	    0x01    /* DMA Start/Stop */

#define DMAB_Active	    0
#define DMAB_Error	    1
#define DMAB_Interrupt	    2
#define DMAB_Simplex	    7

#define DMAF_Active	    (1 << DMAB_Active)
#define DMAF_Error	    (1 << DMAB_Error)
#define DMAF_Interrupt	    (1 << DMAB_Interrupt)
#define DMAF_Simplex	    (1 << DMAB_Simplex)

#define Unit(io) ((struct ata_Unit *)(io)->io_Unit)
#define IOStdReq(io) ((struct IOStdReq *)io)

/* Function prototypes */
int ata_WaitBusy(struct ata_Unit *);
int ata_WaitBusyLong(struct ata_Unit *);
int ata_WaitBusySlow(struct ata_Unit *);

void ata_usleep(struct timerequest *, ULONG);
void ata_ResetBus(struct timerequest *, struct ata_Bus *);
void ata_ScanBus(struct ata_Bus *);
void ata_InitUnits(LIBBASETYPEPTR);

int atapi_SendPacket(struct ata_Unit *, APTR, ULONG, BOOL);
int atapi_TestUnitOK(struct ata_Unit *);

int ata_DirectScsi(struct SCSICmd *, struct ata_Unit *);

int ata_InitBusTask(struct ata_Bus *, int);
int ata_InitDaemonTask(LIBBASETYPEPTR);

VOID dma_SetupPRD(struct ata_Unit *, APTR, ULONG, BOOL);
VOID dma_StartDMA(struct ata_Unit *);
VOID dma_StopDMA(struct ata_Unit *);
VOID dma_Init(struct ata_Bus *);

/*
    ATAPI SCSI commands
*/
//#define SCSI_READ10	
//#define SCSI_WRITE10
//#define SCSI_STARTSTOP

struct atapi_Read10
{
    UBYTE   command;
    UBYTE   pad1;
    UBYTE   lba[4];
    UBYTE   pad2;
    UBYTE   len[2];
    UBYTE   pad3[3];
};

struct atapi_Write10
{
    UBYTE   command;
    UBYTE   pad1;
    UBYTE   lba[4];
    UBYTE   pad2;
    UBYTE   len[2];
    UBYTE   pad3[3];
};

#define ATAPI_SS_EJECT	0x02
#define ATAPI_SS_LOAD	0x03

struct atapi_StartStop
{
    UBYTE   command;
    UBYTE   immediate;
    UBYTE   pad1[2];
    UBYTE   flags;
    UBYTE   pad2[7];
};

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

#endif // _ATA_H


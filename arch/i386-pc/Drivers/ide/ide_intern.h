/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef  IDE_INTERN_H
#define  IDE_INTERN_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/devices.h>
#include <exec/tasks.h>
#include <devices/scsidisk.h>
#include <devices/timer.h>
#include <devices/newstyle.h>
#include <string.h>

/* Stack size - 4096 longwords should be enough */
#define STACK_SIZE      16384

#define MAX_UNIT        8

struct TaskData
{
    struct MsgPort      td_Port;                // MessagePort
    BYTE                td_Flags;               // Flags, see unit flags
    BYTE                td_pad[3];
    struct Task         td_Task;                // Task Structure
    BYTE                td_Stack[STACK_SIZE];   // Keep stack here
};

struct DaemonData
{
    struct Task         dd_Task;                // Task Structure
    struct MsgPort      *dd_TimerMP;            // MessagePort for timer.device
    struct timerequest  *dd_TimerIO;            // IORequest for timer.device
    struct MsgPort      *dd_DevMP;              // MessagePort for chg units.
    struct IOStdReq     *dd_DevIO[MAX_UNIT];    // IOStdReq for each unit
    BYTE                dd_Stack[STACK_SIZE];   // Keep stack here
};

struct ide_Unit
{
    struct Unit         au_Unit;            /* Standard unit */
    struct ideBase      *au_Device;         /* ide.device */
    ULONG           	(*au_ReadSub)();    /* Read function (ATA/ATAPI) */
    ULONG               (*au_WriteSub)();   /* Write function (ATA/ATAPI) */
    ULONG               (*au_SeekSub)();    /* Seek function (ATA/ATAPI) */
    ULONG               (*au_EjectSub)();   /* Eject media function */
    ULONG               (*au_ScsiSub)();    /* SCSI direct */
    
    struct Interrupt    *au_RemoveInt;      /* Remove interrupt */
    struct List         au_SoftList;        /* List of remove ints */

    /* Drive information */    

    char                au_ModelID[32];     /* Model name */
    char                au_RevNumber[4];    /* Version number */
    char                au_SerNumber[12];   /* Serial number */
    ULONG               au_Blocks;          /* Total number of blocks */
    ULONG               au_SectSize;        /* Sector size in bytes */
    UWORD               au_Heads;           /* Number of heads */
    UWORD               au_SectorsT;        /* Sectors/Track */
    UWORD               au_Cylinders;
    UWORD               au_SectorsC;        /* Sectors/Cyl */
    UWORD               au_PortAddr;        /* IO addres of drive */
    ULONG               au_NumLoop;         /* device timeout */
    ULONG               au_ChangeNum;       /* Number of disk changes */
    ULONG		au_CurrSect;	    /* Current sector address */

    UBYTE               au_DevMask;         /* 0x00 - Master, 0x10 - Slave */
    UBYTE               au_OldDevMask;

    UBYTE               au_RDBSector;       /* Number of sector with RDB data */
    UBYTE               au_SecShift;        /* Sector shift (9 for 512 byte) */

    UBYTE               au_CtrlNumber;      /* Number of controller */
    UBYTE               au_UnitNumber;
    
    UWORD               au_SenseKey;
    ULONG               au_LBASense;
    
    ULONG               au_Flags;           /* Flags. See Below */

    UBYTE               au_DevType;         /* Device type. See devices/trackdisk.h */
};

struct iDev
{
    UWORD           idev_DInfo;             /* 0     Device ID */
    UWORD           idev_Cylinders;         /* 1     Total Cyl number */
    UWORD           idev_Rsrvd1;            /* 2     Reserved */
    UWORD           idev_Heads;             /* 3     Number of heads */
    UWORD           idev_BPT;               /* 4     Bytes per track */
    UWORD           idev_BPS;               /* 5     Bytes per sector */
    UWORD           idev_Sectors;           /* 6     Number of sectors */
    UWORD           idev_Rsrvd2[3];         /* 7-9   Reserved */
    char            idev_SerialNumber[20];  /* 10-19 Disk SN */
    UWORD           idev_BufType;           /* 20    Buffer type */
    UWORD           idev_BufSize;           /* 21    Buffer size in 512 byte blocks */
    UWORD           idev_ECCSize;           /* 22    ECC code size */
    char            idev_RevisionNumber[8]; /* 23-26 Drive Revision Number */
    char            idev_ModelNumber[40];   /* 27-46 Model name */
    UWORD           idev_RWMultipleSize;    /* 47    Max sector count for R/W multiple */
    UWORD           idev_32Bit;             /* 48    Is 32 bit mode allowed? */
    UWORD           idev_Features;          /* 49    Drive features */
    UWORD           idev_Rsrvd3;            /* 50    Reserved */
    UWORD           idev_PIOTime;           /* 51    Pio timing */
    UWORD           idev_DMATime;           /* 52    DMA Timing */
    UWORD           idev_NextAvail;         /* 53    Next bytes available? */
    UWORD           ideva_Cylinders;	    /* 54    Number of logical cylinders */
    UWORD           ideva_Heads;	    	/* 55    Number of logical heads */
    UWORD           ideva_Sectors;	    	/* 56    Number of logical sectors */
    UWORD           ideva_Capacity1;	    /* 57    Current capacity in sectors */
    UWORD           ideva_Capacity2;        /* 58    Current capacity */
    UWORD           ideva_MultSect;         /* 59    Multiple sector settings */
    ULONG           ideva_LBASectors;       /* 60-61 LBA reported sectors */
    UWORD           ideva_Rsrvd4[194];
};

#define IB_NA_LOGICAL	0
#define IB_NA_TRANSMODE	1

#define IF_NA_LOGICAL	(1L << IB_NA_LOGICAL)
#define IF_NA_TRANSMODE (1L << IB_NA_TRANSMODE)

struct PartEntry
{
	UBYTE Status;				/* Bootable = 0x80 */
	UBYTE StartH;				/* Start: Head */
	UBYTE StartS;				/* Start: 0:5 Sectors 6:7 Cyls MSB */
	UBYTE StartC;				/* Start: Cyls LBS */
    UBYTE PartType;				/* Partition type code */
    UBYTE EndH;					/* End: Head */
    UBYTE EndS;					/* End: 0:5 Sectors 6:7 Cyls MSB */
    UBYTE EndC;					/* End: Cyls LSB */
    ULONG LBAStart;				/* LBA Starting sector */
    ULONG LBACount;				/* LBA Ending sector */
} __attribute__((packed));		/* This HAS to be packed */

#define IDE_DEVTYPE_NONE    0x00
#define IDE_DEVTYPE_UNKNOWN 0x01
#define IDE_DEVTYPE_ATA     0x02
#define IDE_DEVTYPE_ATAPI   0x80

struct ide_Bus 
{
        UWORD    ib_Port;      /* Base address of the IDE port */
        UBYTE    ib_Dev0;      /* Device 0 type */
        UBYTE    ib_Dev1;      /* Device 1 type */
};

/* Unit flags */
#define	AB_DiskPresent      	31
#define	AB_IntDisable       	30
#define	AB_Swapped          	29
#define	AB_SlowDevice       	28
#define	AB_FastRead         	27
#define	AB_FastWrite        	26
#define	AB_Used             	25
#define	AB_Removable        	23
#define AB_LBAMode          	22
#define	AB_AtapiDev         	21
#define AB_DiskPresenceUnknown	20

#define AF_DiskPresent      	(1L << AB_DiskPresent)
#define AF_IntDisable       	(1L << AB_IntDisable)
#define AF_Swapped          	(1L << AB_Swapped)
#define AF_SlowDevice       	(1L << AB_SlowDevice)
#define AF_FastRead         	(1L << AB_FastRead)
#define AF_FastWrite        	(1L << AB_FastWrite)
#define AF_Used             	(1L << AB_Used)
#define AF_Removable        	(1L << AB_Removable)
#define AF_LBAMode          	(1L << AB_LBAMode)
#define AF_AtapiDev         	(1L << AB_AtapiDev)
#define AF_DiskPresenceUnknown	(1L << AB_DiskPresenceUnknown)

struct ideBase
{
    struct Device           ide_device;
    struct ExecBase         *ide_SysLib;
    BPTR                    ide_SegList;
    ULONG                   ide_NumLoop;
    struct timerequest      *ide_TimerIO;
    struct MsgPort          *ide_TimerMP;
    ULONG                   ide_NumUnit;
    struct TaskData         *ide_TaskData;       // Based on struct Task
    struct DaemonData       *ide_DaemonData;
    struct ide_Unit         *ide_Units[8];
    ULONG                   *ide_BoardAddr;
    UBYTE                   *ide_DevMaskArray;
    struct SignalSemaphore  ide_HardwareLock;	 // stegerg: protect hw accesses
};

#define ata_Error       1
#define ata_SectorCnt   2
#define ata_SectorNum   3
#define ata_CylinderL   4
#define ata_CylinderH   5
#define ata_DevHead     6
#define ata_Status      7
#define ata_Command     7
#define ata_AltStatus   0x206
#define ata_Control     0x206

#define atapi_Error     1
#define atapi_Features  1
#define atapi_Reason    2
#define atapi_ByteCntL  4
#define atapi_ByteCntH  5
#define atapi_DriveSel  6
#define atapi_Status    7
#define atapi_Command   7

/* ATA Commands */
#define ATA_IDENTDEV    0xec
#define ATA_READ		0x20
#define ATA_WRITE       0x30
#define ATA_SEEK        0x70
#define ATA_NOP			0x00
#define ATA_MEDIAEJECT  0xed
#define ATA_RECALIBRATE 0x10

#define ATAB_SLAVE      4
#define ATAB_LBA		6
#define ATAB_ATAPI      7
#define ATAB_DATAREQ    3
#define ATAB_ERROR      0
#define ATAB_BUSY       7

#define ATAF_SLAVE      0x10
#define ATAF_LBA		0x40
#define ATAF_ATAPI      0x80
#define ATAF_DATAREQ    0x08
#define ATAF_ERROR      0x01
#define ATAF_BUSY       0x80

#define ATAPIB_CHECK    0
#define ATAPIF_CHECK    0x01

#define ATAPI_RESET     0x08
#define ATAPI_IDENTDEV  0xa1
#define ATAPI_PACKET    0xa0

#define ATAPIF_MASK     0x03
#define ATAPIF_COMMAND  0x01
#define ATAPIF_READ     0x02
#define ATAPIF_WRITE    0x00

#define APCMD_TESTCHANGED   0x001d
#define APCMD_UNITPARAMS    0x001e

/* Flags for idev_Features */
#define ATAB_FEAT_DMA      8   // Supports DMA
#define ATAB_FEAT_LBA      9   // Supports LBA adressing
#define ATAB_FEAT_IORDYDIS 10  // IORDY can be disabled
#define ATAB_FEAT_IORDYSUP 11  // IORDY might be supported

#define ATAF_FEAT_DMA      (1L << ATAB_FEAT_DMA)
#define ATAF_FEAT_LBA      (1L << ATAB_FEAT_LBA)
#define ATAF_FEAT_IORDYDIS (1L << ATAB_FEAT_IORDYDIS)
#define ATAF_FEAT_IORDYSUP (1L << ATAB_FEAT_IORDYSUP)

/* Flags for idev_NextAvail */
#define ATAB_AVAIL_TCHS    0   // ideva_CHS is available
#define ATAB_AVAIL_TMODE   1   // Transfermode words are valid

#define ATAF_AVAIL_TCHS    (1L << ATAB_AVAIL_TCHS)
#define ATAF_AVAIL_TMODE   (1L << ATAB_AVAIL_TMODE)

#define ide_out(value, offset, port)    outb(value, offset + port)
#define ide_in(offset, port)            inb(offset + port)

/* Function prototypes */
ULONG               TestDevice(struct timerequest *, ULONG, ULONG);
ULONG               TestMirror(ULONG);
void                ScanBus(struct ide_Bus *);
struct ide_Unit     *InitUnit(ULONG, struct ideBase *);
void                UnitInfo(struct ide_Unit *);
void                PerformIO(struct IORequest *, struct ide_Unit *, struct TaskData *);
ULONG               ReadBlocks(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act);
ULONG               WriteBlocks(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act);
ULONG               SendPacket(struct ide_Unit *, ULONG, APTR);
ULONG               WaitBusySlow(ULONG, struct ide_Unit *);

/* HW function prototypes */
ULONG ata_Read(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act);
ULONG ata_Write(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act);
ULONG ata_Seek(ULONG block, struct ide_Unit *unit);
ULONG ata_Eject(struct ide_Unit *unit);
ULONG ata_Identify(APTR buffer, struct ide_Unit *unit);
LONG ata_ScsiCmd(struct SCSICmd *, struct ide_Unit *);

ULONG atapi_TestUnit(struct ide_Unit *unit);
ULONG atapi_Read(ULONG, ULONG, APTR, struct ide_Unit *, ULONG *);
ULONG atapi_Write(ULONG, ULONG, APTR, struct ide_Unit *, ULONG *);
ULONG atapi_Seek(ULONG, struct ide_Unit *);
ULONG atapi_Eject(struct ide_Unit *);

/* Globals */
static const char name[];


/**** ATAPI packets ***********************************************************/

struct atapi_Read10
{
    UBYTE opcode;       /* = 0x28 */
    UBYTE rsvd1;
    UBYTE LBA[4];       /* LBA[0] = MSB, LBA[3] = LSB */
    UBYTE rsvd2;
    UBYTE Len[2];       /* Len[0] = MSB */
    UBYTE rsvd3[3];
};

struct atapi_Write10
{
    UBYTE opcode;       /* = 0x2a */
    UBYTE rsvd1;
    UBYTE LBA[4];       /* LBA[0] = MSB */
    UBYTE rsvd2;
    UBYTE Len[2];       /* Len[0] = MSB */
    UBYTE rsvd3[3];
};

struct atapi_Seek10
{
    UBYTE opcode;       /* = 0x2b */
    UBYTE rsvd1;
    UBYTE LBA[4];       /* LBA[0] = MSB */
    UBYTE rsvd2[6];
};

#define ATAPI_SS_EJECT  0x02
#define ATAPI_SS_LOAD   0x03

struct atapi_StartStop
{
    UBYTE opcode;       /* = 0x1b */
    UBYTE immed;
    UBYTE rsvd1[2];
    UBYTE flgs;
    UBYTE rsvd2[7];
};

/**** IO section **************************************************************/

/* This part SHOULD BE somewhere else!!! */

#define __SLOW_DOWN_IO "\noutb %%al,$0x80"
#define __FULL_SLOW_DOWN_IO __SLOW_DOWN_IO

/*
 * Talk about misusing macros..
 */
#define __OUT1(s,x) \
static inline void out##s(unsigned x value, unsigned short port) {

#define __OUT2(s,s1,s2) \
__asm__ __volatile__ ("out" #s " %" s1 "0,%" s2 "1"

#define __OUT(s,s1,x) \
__OUT1(s,x) __OUT2(s,s1,"w") : : "a" (value), "Nd" (port)); } \
__OUT1(s##_p,x) __OUT2(s,s1,"w") __FULL_SLOW_DOWN_IO : : "a" (value), "Nd" (port));} \

#define __IN1(s) \
static inline RETURN_TYPE in##s(unsigned short port) { RETURN_TYPE _v;

#define __IN2(s,s1,s2) \
__asm__ __volatile__ ("in" #s " %" s2 "1,%" s1 "0"

#define __IN(s,s1,i...) \
__IN1(s) __IN2(s,s1,"w") : "=a" (_v) : "Nd" (port) ,##i ); return _v; } \
__IN1(s##_p) __IN2(s,s1,"w") __FULL_SLOW_DOWN_IO : "=a" (_v) : "Nd" (port) ,##i ); return _v; } \

#define __INS(s) \
static inline void ins##s(unsigned short port, void * addr, unsigned long count) \
{ __asm__ __volatile__ ("cld ; rep ; ins" #s \
: "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count)); }

#define __OUTS(s) \
static inline void outs##s(unsigned short port, const void * addr, unsigned long count) \
{ __asm__ __volatile__ ("cld ; rep ; outs" #s \
: "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count)); }

#define RETURN_TYPE unsigned char
__IN(b,"")
#undef RETURN_TYPE
#define RETURN_TYPE unsigned short
__IN(w,"")
#undef RETURN_TYPE
#define RETURN_TYPE unsigned int
__IN(l,"")
#undef RETURN_TYPE

__OUT(b,"b",char)
__OUT(w,"w",short)
__OUT(l,,int)

__INS(b)
__INS(w)
__INS(l)

__OUTS(b)
__OUTS(w)
__OUTS(l)

#define expunge() \
AROS_LC0(BPTR, expunge, struct TrackDiskBase *, TDBase, 3, TrackDisk)

#ifdef	SysBase
#undef	SysBase
#endif	/* SysBase */
#define	SysBase (*(APTR*)4L)

#endif /* IDE_INTERN_H */


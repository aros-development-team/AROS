#ifndef  IDE_INTERN_H
#define  IDE_INTERN_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/devices.h>
#include <exec/tasks.h>
#include <devices/timer.h>
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

    struct Interrupt    *au_RemoveInt;      /* Remove interrupt */
    struct List         au_SoftList;        /* List of remove ints */

    /* Drive information */    

    char                au_ModelID[32];     /* Model name */
    ULONG               au_Blocks;          /* Total number of blocks */
    ULONG               au_SectSize;        /* Sector size in bytes */
    UBYTE               au_Heads;           /* Number of heads */
    UBYTE               au_SectorsT;        /* Sectors/Track */
    UWORD               au_Cylinders;
    UWORD               au_SectorsC;        /* Sectors/Cyl */
    UWORD               au_PortAddr;        /* IO addres of drive */
    ULONG               au_NumLoop;         /* device timeout */
    ULONG               au_ChangeNum;       /* Number of disk changes */

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
    UWORD           idev_DInfo;             /* Device ID */
    UWORD           idev_Cylinders;         /* Total Cyl number */
    UWORD           idev_Rsrvd1;
    UWORD           idev_Heads;             /* Number of heads */
    UWORD           idev_BPT;               /* Bytes per track */
    UWORD           idev_BPS;               /* Bytes per sector */
    UWORD           idev_Sectors;           /* Number of sectors */
    UWORD           idev_Rsrvd2[3];
    char            idev_SerialNumber[20];  /* Disk SN */
    UWORD           idev_BufType;           /* Buffer type */
    UWORD           idev_BufSize;           /* Buffer size in 512 byte blocks */
    UWORD           idev_ECCSize;           /* ECC code size */
    char            idev_RevisionNumber[8]; /* Drive Revision Number */
    char            idev_ModelNumber[40];   /* Model name */
    UWORD           idev_RWMultipleSize;    /* Max sector count for R/W Multiple */
    UWORD           idev_32Bit;             /* Is 32 bit mode allowed? */
    UWORD           idev_Features;          /* Drive features */
    UWORD           idev_Rsrvd3;
    UWORD           idev_PIOTime;           /* Pio timing */
    UWORD           idev_DMATime;           /* DMA Timing */
    UWORD           idev_NextAvail;         /* Next bytes available? */
    UWORD           ideva_Cylinders;
    UWORD           ideva_Heads;
    UWORD           ideva_Sectors;
    ULONG           ideva_Capacity;
    UWORD           ideva_Rsrvd4[197];
};

/* Unit flags */
#define	AB_DiskPresent  31
#define	AB_IntDisable   30
#define	AB_Swapped      29
#define	AB_SlowDevice   28
#define	AB_FastRead     27
#define	AB_FastWrite    26
#define	AB_Used         25
#define	AB_Removable    23
#define AB_LBAMode      22
#define	AB_AtapiDev     21

#define AF_DiskPresent  (1L << AB_DiskPresent)
#define AF_IntDisable   (1L << AB_IntDisable)
#define AF_Swapped      (1L << AB_Swapped)
#define AF_SlowDevice   (1L << AB_SlowDevice)
#define AF_FastRead     (1L << AB_FastRead)
#define AF_FastWrite    (1L << AB_FastWrite)
#define AF_Used         (1L << AB_Used)
#define AF_Removable    (1L << AB_Removable)
#define AF_LBAMode      (1L << AB_LBAMode)
#define AF_AtapiDev     (1L << AB_AtapiDev)

struct ideBase
{
    struct Device       ide_device;
    struct ExecBase     *ide_SysLib;
    BPTR                ide_SegList;
    ULONG               ide_NumLoop;
    struct timerequest  *ide_TimerIO;
    struct MsgPort      *ide_TimerMP;
    ULONG               ide_NumUnit;
    struct TaskData     *ide_TaskData;       // Based on struct Task
    struct DaemonData   *ide_DaemonData;
    struct ide_Unit     *ide_Units[8];
    ULONG               *ide_BoardAddr;
    UBYTE               *ide_DevMaskArray;
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
#define ATA_READ	    0x20
#define ATA_WRITE       0x30
#define ATA_SEEK        0x70
#define ATA_NOP		    0x00
#define ATA_MEDIAEJECT  0xed
#define ATA_RECALIBRATE 0x10

#define ATAB_LBA	    6
#define ATAB_ATAPI      7
#define ATAB_DATAREQ    3
#define ATAB_ERROR      0
#define ATAB_BUSY       7

#define ATAF_LBA	    0x40
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

#define ide_out(value, offset, port)    outb(value, offset + port)
#define ide_in(offset, port)            inb(offset + port)

#define NSCMD_TD_READ64     0xc000
#define NSCMD_TD_WRITE64    0xc001
#define NSCMD_TD_SEEK64     0xc002
#define NSCMD_TD_FORMAT64   0xc003

#define APCMD_TESTCHANGED   0x001d
#define APCMD_UNITPARAMS    0x001e
#define NSCMD_DEVICEQUERY   0x4000

/**** ATAPI packets ***********************************************************/

#define ATAPI_READ10    0x28
#define ATAPI_WRITE10   0x2a
#define ATAPI_SEEK10    0x2b
#define ATAPI_STARTSTOP 0x1b

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
extern inline void out##s(unsigned x value, unsigned short port) {

#define __OUT2(s,s1,s2) \
__asm__ __volatile__ ("out" #s " %" s1 "0,%" s2 "1"

#define __OUT(s,s1,x) \
__OUT1(s,x) __OUT2(s,s1,"w") : : "a" (value), "Nd" (port)); } \
__OUT1(s##_p,x) __OUT2(s,s1,"w") __FULL_SLOW_DOWN_IO : : "a" (value), "Nd" (port));} \

#define __IN1(s) \
extern inline RETURN_TYPE in##s(unsigned short port) { RETURN_TYPE _v;

#define __IN2(s,s1,s2) \
__asm__ __volatile__ ("in" #s " %" s2 "1,%" s1 "0"

#define __IN(s,s1,i...) \
__IN1(s) __IN2(s,s1,"w") : "=a" (_v) : "Nd" (port) ,##i ); return _v; } \
__IN1(s##_p) __IN2(s,s1,"w") __FULL_SLOW_DOWN_IO : "=a" (_v) : "Nd" (port) ,##i ); return _v; } \

#define __INS(s) \
extern inline void ins##s(unsigned short port, void * addr, unsigned long count) \
{ __asm__ __volatile__ ("cld ; rep ; ins" #s \
: "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count)); }

#define __OUTS(s) \
extern inline void outs##s(unsigned short port, const void * addr, unsigned long count) \
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


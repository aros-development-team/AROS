#ifndef  TRACKDISK_INTERN_H
#define  TRACKDISK_INTERN_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/devices.h>

struct TrackDiskBase
{
    struct Device      		td_device;
    struct ExecBase		*sysbase;	/* Useless for native but... */
    struct SignalSemaphore	io_lock;	/* Lock IO acces to floppy */
    struct TDU			*units[4];	/* Up to four units allowed */
    UBYTE			DOR;		/* Digital Output Register */
    UBYTE			rawcom[9];	/* RAW command to send */
    UBYTE			result[7];	/* Last set of bytes */
};

#define TDU_P(x) ((struct TDU_PublicUnit *)x)

struct TDU
{
    struct	TDU_PublicUnit;
    UBYTE 	unitnum;		/* Unit number */
    UBYTE 	unittype;		/* Unit type from BIOS setup */
    APTR	dma_buffer;		/* Buffer for DMA transfers */
    UBYTE	head;			/* Active head */
};

/*
   Drive parameters.
   There is only one possible drive type at this moment - 1.44MB 3.5" floppy.
   5.25" floppies are not supported at this moment. In future one should add other
   drives support (including 2.88MB drives).
*/

#define DP_STOTAL	2880	/* Sectors total */
#define DP_SECTORS	18	/* Sectors per cyl */
#define DP_SIDES	2	/* No of sides */
#define DP_TRACKS	80
#define DP_GAP1		0x1b	/* Gap for reading */
#define DP_SPEC1	0xc1	/* SRT=4ms, HUT=16ms */
#define	DP_GAP2		0x6c	/* Gap for formatting */
#define DP_SPEC2	0x10	/* HLT=16ms, use DMA */
#define	DP_SSIZE	0x02	/* SectSize=512b */

/* TrackDisk i82072 commands */

#define i82_READ	0x06
#define	i82_WRITE	0x05
#define i82_FORMAT	0x0d
#define i82_RECALIBRATE	0x07
#define	i82_SEEK	0x0f
#define i82_RELSEEK	0x8f

#define fd_inb(port)		inb_p(port)
#define fd_outb(value,port)     outb_p(value,port)


#define FD_STATUS   0x3f4
#define FD_DATA     0x3f5
#define FD_DOR      0x3f2       /* Digital Output Register */
#define FD_DIR      0x3f7       /* Digital Input Register (read) */
#define FD_DCR      0x3f7       /* Diskette Control Register (write)*/

/* Bits of main status register */
#define STATUS_BUSYMASK 0x0F        /* drive busy mask */
#define STATUS_BUSY 0x10        /* FDC busy */
#define STATUS_DMA  0x20        /* 0- DMA mode */
#define STATUS_DIR  0x40        /* 0- cpu->fdc */
#define STATUS_READY    0x80        /* Data reg ready */

/* Bits of FD_ST0 */
#define ST0_DS      0x03        /* drive select mask */
#define ST0_HA      0x04        /* Head (Address) */
#define ST0_NR      0x08        /* Not Ready */
#define ST0_ECE     0x10        /* Equipment check error */
#define ST0_SE      0x20        /* Seek end */
#define ST0_INTR    0xC0        /* Interrupt code mask */

/* Bits of FD_ST1 */
#define ST1_MAM     0x01        /* Missing Address Mark */
#define ST1_WP      0x02        /* Write Protect */
#define ST1_ND      0x04        /* No Data - unreadable */
#define ST1_OR      0x10        /* OverRun */
#define ST1_CRC     0x20        /* CRC error in data or addr */
#define ST1_EOC     0x80        /* End Of Cylinder */

/* Bits of FD_ST2 */
#define ST2_MAM     0x01        /* Missing Address Mark (again) */
#define ST2_BC      0x02        /* Bad Cylinder */
#define ST2_SNS     0x04        /* Scan Not Satisfied */
#define ST2_SEH     0x08        /* Scan Equal Hit */
#define ST2_WC      0x10        /* Wrong Cylinder */
#define ST2_CRC     0x20        /* CRC error in data field */
#define ST2_CM      0x40        /* Control Mark = deleted */

/* Bits of FD_ST3 */
#define ST3_HA      0x04        /* Head (Address) */
#define ST3_DS      0x08        /* drive is double-sided */
#define ST3_TZ      0x10        /* Track Zero signal (1=track 0) */
#define ST3_RY      0x20        /* drive is ready */
#define ST3_WP      0x40        /* Write Protect */
#define ST3_FT      0x80        /* Drive Fault */

/* Values for FD_COMMAND */
#define FD_RECALIBRATE      0x07    /* move to track 0 */
#define FD_SEEK         0x0F    /* seek track */
#define FD_READ         0xE6    /* read with MT, MFM, SKip deleted */
#define FD_WRITE        0xC5    /* write with MT, MFM */
#define FD_SENSEI       0x08    /* Sense Interrupt Status */
#define FD_SPECIFY      0x03    /* specify HUT etc */
#define FD_FORMAT       0x4D    /* format one track */
#define FD_VERSION      0x10    /* get version code */
#define FD_CONFIGURE        0x13    /* configure FIFO operation */
#define FD_PERPENDICULAR    0x12    /* perpendicular r/w mode */
#define FD_GETSTATUS        0x04    /* read ST3 */
#define FD_DUMPREGS     0x0E    /* dump the contents of the fdc regs */
#define FD_READID       0xEA    /* prints the header of a sector */
#define FD_UNLOCK       0x14    /* Fifo config unlock */
#define FD_LOCK         0x94    /* Fifo config lock */
#define FD_RSEEK_OUT        0x8f    /* seek out (i.e. to lower tracks) */
#define FD_RSEEK_IN     0xcf    /* seek in (i.e. to higher tracks) */

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

#define expunge() \
AROS_LC0(BPTR, expunge, struct TrackDiskBase *, TDBase, 3, TrackDisk)

#ifdef	SysBase
#undef	SysBase
#endif	/* SysBase */
#define	SysBase (*(APTR*)4L)

#endif /* KEYBOARD_INTERN_H */

















#ifndef  TRACKDISK_HW_H
#define  TRACKDISK_HW_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hardware defs for trackdisk
    Lang: English
*/

#include <exec/types.h>
#include <asm/io.h>

/* Prototypes */
void td_motoron(UBYTE,struct TrackDiskBase *,BOOL);
void td_select(UBYTE,struct TrackDiskBase *);
void td_motoroff(UBYTE,struct TrackDiskBase *);
UBYTE td_getprotstatus(UBYTE,struct TrackDiskBase *);
int td_dinit(struct TrackDiskBase *);
int td_recalibrate(unsigned char, char, int, struct TrackDiskBase *);
int td_rseek(UBYTE , UBYTE , UBYTE , struct TrackDiskBase *);
int td_read(struct IOExtTD *, struct TrackDiskBase *);
int td_write(struct IOExtTD *, struct TrackDiskBase *);
int td_update(struct TDU *, struct TrackDiskBase *);
int td_format(struct IOExtTD *, struct TrackDiskBase *);
int td_waitint(struct TrackDiskBase *,UBYTE,BOOL);
UBYTE td_getDiskChange(void);


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

/* I/O ports used by the FDC */
#define FDC_SRB	    0x3f1	/* Status register B */
#define FDC_DOR	    0x3f2	/* Digital output register */
#define FDC_TDR	    0x3f3	/* Tape drive register */
#define FDC_MSR	    0x3f4	/* Main status register (R) */
#define FDC_DSR	    0x3f4	/* Data rate select (W) */
#define FDC_FIFO    0x3f5	/* FIFO */
#define FDC_DIR	    0x3f7	/* Digital input register (R) */
#define FDC_CCR	    0x3f7	/* Configuration control register (W) */

/* Status register B bits */
#define SRBB_IDLE	0x00
#define SRBB_PD		0x01
#define SRBB_IDLEMSK	0x02

#define SRBF_IDLE	(1<<SRBB_IDLE)
#define SRBF_PD		(1<<SRBB_PD)
#define SRBF_IDLEMSK	(1<<SRBB_IDLEMSK)

/* Digital output register bits */
#define DORB_DSEL	0x00
#define DORB_RESET	0x02
#define DORB_DMA	0x03
#define DORB_MOT0	0x04
#define DORB_MOT1	0x05

#define DORF_DSEL	(1<<DORB_DSEL)
#define DORF_RESET	(1<<DORB_RESET)
#define DORF_DMA	(1<<DORB_DMA)
#define DORF_MOT0	(1<<DORB_MOT0)
#define DORF_MOT1	(1<<DORB_MOT1)

/* Tape drive register bits */
#define TDRB_TS0	0x00
#define TDRB_TS1	0x01
#define TDRB_BSEL	0x02

#define TDRF_TS0	(1<<TDRB_TS0)
#define TDRF_TS1	(1<<TDRB_TS1)
#define TDRF_BSEL	(1<<TDRB_BSEL)

/* Main status register bits */
#define MSRB_D0BSY	0x00
#define MSRB_D1BSY	0x01
#define MSRB_CMDBSY	0x04
#define MSRB_NONDMA	0x05
#define MSRB_DIO	0x06
#define MSRB_RQM	0x07

#define MSRF_D0BSY	(1<<MSRB_D0BSY)
#define MSRF_D1BSY	(1<<MSRB_D1BSY)
#define MSRF_CMDBSY	(1<<MSRB_CMDBSY)
#define MSRF_NONDMA	(1<<MSRB_NONDMA)
#define MSRF_DIO	(1<<MSRB_DIO)
#define MSRF_RQM	(1<<MSRB_RQM)

/* Datarate select bits */
#define DSRB_DRATE0	0x00
#define DSRB_DRATE1	0x01
#define DSRB_PCOMP0	0x02
#define DSRB_PCOMP1	0x03
#define DSRB_PCOMP2	0x04
#define DSRB_PDOSC	0x05
#define DSRB_PDOWN	0x06
#define DSRB_RESET	0x07

#define DSRF_DRATE0	(1<<DSRB_DRATE0)
#define DSRF_DRATE1	(1<<DSRB_DRATE1)
#define DSRF_PCOMP0	(1<<DSRB_PCOMP0)
#define DSRF_PCOMP1	(1<<DSRB_PCOMP1)
#define DSRF_PCOMP2	(1<<DSRB_PCOMP2)
#define DSRF_PDOSC	(1<<DSRB_PDOSC)
#define DSRF_PDOWN	(1<<DSRB_PDOWN)
#define DSRF_RESET	(1<<DSRB_RESET)

/* Digital input register bits */
#define DIRB_DCHG	0x07

#define DIRF_DCHG	(1<<DIRB_DCHG)

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
#define FD_RECALIBRATE	    0x07    /* move to track 0 */
#define FD_SEEK		    0x0F    /* seek track */
#define FD_READ		    0x66    /* read with MFM, SKip deleted */
#define FD_WRITE	    0x45    /* write with MFM */
#define FD_SENSEI	    0x08    /* Sense Interrupt Status */
#define FD_SPECIFY	    0x03    /* specify HUT etc */
#define FD_FORMAT	    0x4D    /* format one track */
#define FD_VERSION	    0x10    /* get version code */
#define FD_PARTID	    0x18    /* get part id */
#define FD_CONFIGURE        0x13    /* configure FIFO operation */
#define FD_PERPENDICULAR    0x12    /* perpendicular r/w mode */
#define FD_GETSTATUS        0x04    /* read ST3 */
#define FD_DUMPREGS	    0x0E    /* dump the contents of the fdc regs */
#define FD_READID	    0x4A    /* prints the header of a sector */
#define FD_UNLOCK	    0x14    /* Fifo config unlock */
#define FD_LOCK		    0x94    /* Fifo config lock */
#define FD_RSEEK_OUT        0x8F    /* seek out (i.e. to lower tracks) */
#define FD_RSEEK_IN	    0xCF    /* seek in (i.e. to higher tracks) */
#define FD_PDMODE	    0x17    /* Configure powerdown mode */



/*
 * This really should be somewhere else, like in a hidd.dma or so
 * And it should not look like this, considering licenses etc.
 */

/* DMA section based on linuxish /asm/dma.h */

#define dma_outb		outb
#define dma_inb			inb
#define MAX_DMA_CHANNELS	8

/* The maximum address that we can perform a DMA transfer to on this platform */
#define MAX_DMA_ADDRESS      (PAGE_OFFSET+0x1000000)

/* 8237 DMA controllers */
#define IO_DMA1_BASE	0x00	/* 8 bit slave DMA, channels 0..3 */
#define IO_DMA2_BASE	0xC0	/* 16 bit master DMA, ch 4(=slave input)..7 */

/* DMA controller registers */
#define DMA1_CMD_REG		0x08	/* command register (w) */
#define DMA1_STAT_REG		0x08	/* status register (r) */
#define DMA1_REQ_REG            0x09    /* request register (w) */
#define DMA1_MASK_REG		0x0A	/* single-channel mask (w) */
#define DMA1_MODE_REG		0x0B	/* mode register (w) */
#define DMA1_CLEAR_FF_REG	0x0C	/* clear pointer flip-flop (w) */
#define DMA1_TEMP_REG           0x0D    /* Temporary Register (r) */
#define DMA1_RESET_REG		0x0D	/* Master Clear (w) */
#define DMA1_CLR_MASK_REG       0x0E    /* Clear Mask */
#define DMA1_MASK_ALL_REG       0x0F    /* all-channels mask (w) */

#define DMA2_CMD_REG		0xD0	/* command register (w) */
#define DMA2_STAT_REG		0xD0	/* status register (r) */
#define DMA2_REQ_REG            0xD2    /* request register (w) */
#define DMA2_MASK_REG		0xD4	/* single-channel mask (w) */
#define DMA2_MODE_REG		0xD6	/* mode register (w) */
#define DMA2_CLEAR_FF_REG	0xD8	/* clear pointer flip-flop (w) */
#define DMA2_TEMP_REG           0xDA    /* Temporary Register (r) */
#define DMA2_RESET_REG		0xDA	/* Master Clear (w) */
#define DMA2_CLR_MASK_REG       0xDC    /* Clear Mask */
#define DMA2_MASK_ALL_REG       0xDE    /* all-channels mask (w) */

#define DMA_ADDR_0              0x00    /* DMA address registers */
#define DMA_ADDR_1              0x02
#define DMA_ADDR_2              0x04
#define DMA_ADDR_3              0x06
#define DMA_ADDR_4              0xC0
#define DMA_ADDR_5              0xC4
#define DMA_ADDR_6              0xC8
#define DMA_ADDR_7              0xCC

#define DMA_CNT_0               0x01    /* DMA count registers */
#define DMA_CNT_1               0x03
#define DMA_CNT_2               0x05
#define DMA_CNT_3               0x07
#define DMA_CNT_4               0xC2
#define DMA_CNT_5               0xC6
#define DMA_CNT_6               0xCA
#define DMA_CNT_7               0xCE

#define DMA_PAGE_0              0x87    /* DMA page registers */
#define DMA_PAGE_1              0x83
#define DMA_PAGE_2              0x81
#define DMA_PAGE_3              0x82
#define DMA_PAGE_5              0x8B
#define DMA_PAGE_6              0x89
#define DMA_PAGE_7              0x8A

#define DMA_MODE_READ	0x44	/* I/O to memory, no autoinit, increment, single mode */
#define DMA_MODE_WRITE	0x48	/* memory to I/O, no autoinit, increment, single mode */
#define DMA_MODE_CASCADE 0xC0   /* pass thru DREQ->HRQ, DACK<-HLDA only */

#define DMA_AUTOINIT	0x10

/* enable/disable a specific DMA channel */
static __inline__ void enable_dma(unsigned int dmanr)
{
	if (dmanr<=3)
		dma_outb(dmanr,  DMA1_MASK_REG);
	else
		dma_outb(dmanr & 3,  DMA2_MASK_REG);
}

static __inline__ void disable_dma(unsigned int dmanr)
{
	if (dmanr<=3)
		dma_outb(dmanr | 4,  DMA1_MASK_REG);
	else
		dma_outb((dmanr & 3) | 4,  DMA2_MASK_REG);
}

/* Clear the 'DMA Pointer Flip Flop'.
 * Write 0 for LSB/MSB, 1 for MSB/LSB access.
 * Use this once to initialize the FF to a known state.
 * After that, keep track of it. :-)
 * --- In order to do that, the DMA routines below should ---
 * --- only be used while holding the DMA lock ! ---
 */
static __inline__ void clear_dma_ff(unsigned int dmanr)
{
	if (dmanr<=3)
		dma_outb(0,  DMA1_CLEAR_FF_REG);
	else
		dma_outb(0,  DMA2_CLEAR_FF_REG);
}

/* set mode (above) for a specific DMA channel */
static __inline__ void set_dma_mode(unsigned int dmanr, char mode)
{
	if (dmanr<=3)
		dma_outb(mode | dmanr,  DMA1_MODE_REG);
	else
		dma_outb(mode | (dmanr&3),  DMA2_MODE_REG);
}

/* Set only the page register bits of the transfer address.
 * This is used for successive transfers when we know the contents of
 * the lower 16 bits of the DMA current address register, but a 64k boundary
 * may have been crossed.
 */
static __inline__ void set_dma_page(unsigned int dmanr, char pagenr)
{
	switch(dmanr) {
		case 0:
			dma_outb(pagenr, DMA_PAGE_0);
			break;
		case 1:
			dma_outb(pagenr, DMA_PAGE_1);
			break;
		case 2:
			dma_outb(pagenr, DMA_PAGE_2);
			break;
		case 3:
			dma_outb(pagenr, DMA_PAGE_3);
			break;
		case 5:
			dma_outb(pagenr & 0xfe, DMA_PAGE_5);
			break;
		case 6:
			dma_outb(pagenr & 0xfe, DMA_PAGE_6);
			break;
		case 7:
			dma_outb(pagenr & 0xfe, DMA_PAGE_7);
			break;
	}
}

/* Set transfer address & page bits for specific DMA channel.
 * Assumes dma flipflop is clear.
 */
static __inline__ void set_dma_addr(unsigned int dmanr, unsigned int a)
{
	set_dma_page(dmanr, a>>16);
	if (dmanr <= 3)  {
	    dma_outb( a & 0xff, ((dmanr&3)<<1) + IO_DMA1_BASE );
            dma_outb( (a>>8) & 0xff, ((dmanr&3)<<1) + IO_DMA1_BASE );
	}  else  {
	    dma_outb( (a>>1) & 0xff, ((dmanr&3)<<2) + IO_DMA2_BASE );
	    dma_outb( (a>>9) & 0xff, ((dmanr&3)<<2) + IO_DMA2_BASE );
	}
}


/* Set transfer size (max 64k for DMA1..3, 128k for DMA5..7) for
 * a specific DMA channel.
 * You must ensure the parameters are valid.
 * NOTE: from a manual: "the number of transfers is one more
 * than the initial word count"! This is taken into account.
 * Assumes dma flip-flop is clear.
 * NOTE 2: "count" represents _bytes_ and must be even for channels 5-7.
 */
static __inline__ void set_dma_count(unsigned int dmanr, unsigned int count)
{
        count--;
	if (dmanr <= 3)  {
	    dma_outb( count & 0xff, ((dmanr&3)<<1) + 1 + IO_DMA1_BASE );
	    dma_outb( (count>>8) & 0xff, ((dmanr&3)<<1) + 1 + IO_DMA1_BASE );
        } else {
	    dma_outb( (count>>1) & 0xff, ((dmanr&3)<<2) + 2 + IO_DMA2_BASE );
	    dma_outb( (count>>9) & 0xff, ((dmanr&3)<<2) + 2 + IO_DMA2_BASE );
        }
}

#endif /* TRACKDISK_HW_H */

















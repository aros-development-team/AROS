/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: m68k-amiga hardware registers
    Lang: english
 */

#ifndef AMIGA_HWREG_H
#define AMIGA_HWREG_H

#include <exec/types.h>

/** Interrupts */
#define INTENAR			0x1c
#define INTREQR			0x1e
#define INTENA			0x9a
#define INTREQ			0x9c

/** Screen/Serial Debug **/

#define SERPER_BASE_PAL		3546895
#define SERPER_BASE_NTSC	3579545
#define SERPER			0x32
#define   SERPER_BAUD(base, x)	((((base + (x)/2))/(x)-1) & 0x7fff)	/* Baud rate */
#define SERDATR			0x18
#define   SERDATR_OVRUN		(1 << 15)	/* Overrun */
#define   SERDATR_RBF		(1 << 14)	/* Rx Buffer Full */
#define   SERDATR_TBE		(1 << 13)	/* Tx Buffer Empty */
#define   SERDATR_TSRE		(1 << 12)	/* Tx Shift Empty */
#define   SERDATR_RXD		(1 << 11)	/* Rx Pin */
#define   SERDATR_STP9		(1 <<  9)	/* Stop bit (if 9 data) */
#define   SERDATR_STP8		(1 <<  8)	/* Stop bit (if 8 data) */
#define   SERDATR_DB9_of(x)	((x) & 0x1ff)	/* 9-bit data */
#define   SERDATR_DB8_of(x)	((x) & 0xff)	/* 8-bit data */
#define ADKCON			0x9e
#define   ADKCON_SETCLR		(1 << 15)
#define   ADKCON_UARTBRK	(1 << 11)	/* Force break */
#define SERDAT			0x30
#define   SERDAT_STP9		(1 << 9)
#define   SERDAT_STP8		(1 << 8)
#define   SERDAT_DB9(x)		((x) & 0x1ff)
#define   SERDAT_DB8(x)		((x) & 0xff)

#define BPLCON0			0x100
#define BPL1DAT			0x110
#define COLOR00			0x180

static inline void reg_w(ULONG reg, UWORD val)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	*r = val;
}

static inline UWORD reg_r(ULONG reg)
{
	volatile UWORD *r = (void *)(0xdff000 + reg);

	return *r;
}

#endif /* AMIGA_HWREG_H */

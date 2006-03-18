/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/radeon_macros.h,v 1.2 2003/07/08 15:39:48 tsi Exp $ */
/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
 *                VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <martin@xfree86.org>
 *   Rickard E. Faith <faith@valinux.com>
 *   Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 * References:
 *
 * !!!! FIXME !!!!
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 * !!!! FIXME !!!!
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 */


#ifndef _RADEON_MACROS_H_
#define _RADEON_MACROS_H_

#define RADEONMMIO  (sd->Card.MMIO)

#define RADEON_BIOS8(v)  (sd->Card.VBIOS[v])
#define RADEON_BIOS16(v) (sd->Card.VBIOS[v] | \
                          (sd->Card.VBIOS[(v) + 1] << 8))
#define RADEON_BIOS32(v) (sd->Card.VBIOS[v] | \
                          (sd->Card.VBIOS[(v) + 1] << 8) | \
                          (sd->Card.VBIOS[(v) + 2] << 16) | \
                          (sd->Card.VBIOS[(v) + 3] << 24))

				/* Memory mapped register access macros */
#define INREG8(addr)        (*(volatile UBYTE*)((UBYTE*)(RADEONMMIO)+(addr)))
#define INREG16(addr)       (*(volatile UWORD*)((UBYTE*)(RADEONMMIO)+(addr)))
#define INREG(addr)         (*(volatile ULONG*)((UBYTE*)(RADEONMMIO)+(addr)))
#define OUTREG8(addr, val)  (*(volatile UBYTE*)((UBYTE*)(RADEONMMIO)+(addr)) = (val))
#define OUTREG16(addr, val) (*(volatile UWORD*)((UBYTE*)(RADEONMMIO)+(addr)) = (val))
#define OUTREG(addr, val)   (*(volatile ULONG*)((UBYTE*)(RADEONMMIO)+(addr)) = (val))

#define ADDRREG(addr)       ((volatile ULONG *)(APTR)(RADEONMMIO + (addr)))

#define OUTREGP(addr, val, mask)					\
do {									\
    ULONG tmp = INREG(addr);						\
    tmp &= (mask);							\
    tmp |= (val);							\
    OUTREG(addr, tmp);							\
} while (0)

#define INPLL(sd, addr) RADEONINPLL(sd, addr)

#define OUTPLL(addr, val)						\
do {									\
    OUTREG8(RADEON_CLOCK_CNTL_INDEX, (((addr) & 0x3f) |			\
				      RADEON_PLL_WR_EN));		\
    OUTREG(RADEON_CLOCK_CNTL_DATA, val);				\
} while (0)

#define OUTPLLP(sd, addr, val, mask)					\
do {									\
    ULONG tmp_ = INPLL(sd, addr);					\
    tmp_ &= (mask);							\
    tmp_ |= (val);							\
    OUTPLL(addr, tmp_);							\
} while (0)

#define OUTPAL_START(idx)						\
do {									\
    OUTREG8(RADEON_PALETTE_INDEX, (idx));				\
} while (0)

#define OUTPAL_NEXT(r, g, b)						\
do {									\
    OUTREG(RADEON_PALETTE_DATA, ((r) << 16) | ((g) << 8) | (b));	\
} while (0)

#define OUTPAL_NEXT_CARD32(v)						\
do {									\
    OUTREG(RADEON_PALETTE_DATA, (v & 0x00ffffff));			\
} while (0)

#define OUTPAL(idx, r, g, b)						\
do {									\
    OUTPAL_START((idx));						\
    OUTPAL_NEXT((r), (g), (b));						\
} while (0)

#define INPAL_START(idx)						\
do {									\
    OUTREG(RADEON_PALETTE_INDEX, (idx) << 16);				\
} while (0)

#define INPAL_NEXT() INREG(RADEON_PALETTE_DATA)

#define PAL_SELECT(idx)							\
do {									\
    if (!idx) {								\
	OUTREG(RADEON_DAC_CNTL2, INREG(RADEON_DAC_CNTL2) &		\
	       (ULONG)~RADEON_DAC2_PALETTE_ACC_CTL);			\
    } else {								\
	OUTREG(RADEON_DAC_CNTL2, INREG(RADEON_DAC_CNTL2) |		\
	       RADEON_DAC2_PALETTE_ACC_CTL);				\
    }									\
} while (0)


#endif


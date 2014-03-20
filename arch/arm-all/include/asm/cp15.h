/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The purpose of the system control coprocessor, CP15, is to control and provide status information for the functions implemented in the processor.
    Lang: english
*/

#ifndef ASM_ARM_CP15_H
#define ASM_ARM_CP15_H

#include <exec/types.h>
#include <inttypes.h>

#ifdef __ARM_ARCH_7A__
/* c1, (C)ontrol (R)egister */
#define	C1_CR_M			(1<<0)
#define	C1_CR_A			(1<<1)
#define	C1_CR_C			(1<<2)
#define	C1_CR_Z			(1<<11)
#define	C1_CR_I			(1<<12)
#define	C1_CR_V			(1<<13)
#define	C1_CR_EE		(1<<25)
#define	C1_CR_NMFI		(1<<27)
#define	C1_CR_TRE		(1<<28)
#define	C1_CR_AFE		(1<<29)
#define	C1_CR_TE		(1<<30)
#endif /* __ARM_ARCH_7A__ */

static inline void CP15_CR_Set(uint32_t val) {
	uint32_t __v;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(__v));
    __v |= val;
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(__v));
}

static inline void CP15_CR_Clear(uint32_t val) {
	uint32_t __v;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(__v));
    __v &= ~val;
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(__v));
}

#endif /* ASM_ARM_CP15_H */

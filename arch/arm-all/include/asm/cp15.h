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
#define	C1B_M		0
#define	C1B_A		1
#define	C1B_C		2
#define	C1B_Z		11
#define	C1B_I		12
#define	C1B_V		13
#define	C1B_EE		25
#define	C1B_NMFI	27
#define	C1B_TRE		28
#define	C1B_AFE		29
#define	C1B_TE		30
#define	C1F_M		(1UL<<C1B_M)
#define	C1F_A		(1UL<<C1B_A)
#define	C1F_C		(1UL<<C1B_C)
#define	C1F_Z		(1UL<<C1B_Z)
#define	C1F_I		(1UL<<C1B_I)
#define	C1F_V		(1UL<<C1B_V)
#define	C1F_EE		(1UL<<C1B_EE)
#define	C1F_NMFI	(1UL<<C1B_NMFI)
#define	C1F_TRE		(1UL<<C1B_TRE)
#define	C1F_AFE		(1UL<<C1B_AFE)
#define	C1F_TE		(1UL<<C1B_TE)
#endif /* __ARM_ARCH_7A__ */

static inline void CP15_CR1_Set(uint32_t val) {
	uint32_t __v;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(__v));
    __v |= val;
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(__v));
}

static inline void CP15_CR1_Clear(uint32_t val) {
	uint32_t __v;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(__v));
    __v &= ~val;
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(__v));
}

#endif /* ASM_ARM_CP15_H */

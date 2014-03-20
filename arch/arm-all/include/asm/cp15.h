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
/* C1, (C)ontrol (R)egister */
#define	C1CRB_M		0
#define	C1CRB_A		1
#define	C1CRB_C		2
#define	C1CRB_Z		11
#define	C1CRB_I		12
#define	C1CRB_V		13
#define	C1CRB_EE	25
#define	C1CRB_NMFI	27
#define	C1CRB_TRE	28
#define	C1CRB_AFE	29
#define	C1CRB_TE	30
#define	C1CRF_M		(1UL<<C1CRB_M)
#define	C1CRF_A		(1UL<<C1CRB_A)
#define	C1CRF_C		(1UL<<C1CRB_C)
#define	C1CRF_Z		(1UL<<C1CRB_Z)
#define	C1CRF_I		(1UL<<C1CRB_I)
#define	C1CRF_V		(1UL<<C1CRB_V)
#define	C1CRF_EE	(1UL<<C1CRB_EE)
#define	C1CRF_NMFI	(1UL<<C1CRB_NMFI)
#define	C1CRF_TRE	(1UL<<C1CRB_TRE)
#define	C1CRF_AFE	(1UL<<C1CRB_AFE)
#define	C1CRF_TE	(1UL<<C1CRB_TE)
/* C1 (C)oprocessor (A)ccess (C)ontrol (R)egister */
#define C1CACRF_CPAP 0b00000011111111111111111111111111
#define C1CACRV_CPAP(cp) ((0b11)<<(cp*2))
#endif /* __ARM_ARCH_7A__ */

/* C1 (C)ontrol (R)egister SET */
static inline void CP15_C1CR_Set(uint32_t val) {
	uint32_t __v;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(__v));
    __v |= val;
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(__v));
}

/* C1 (C)ontrol (R)egister CLEAR */
static inline void CP15_C1CR_Clear(uint32_t val) {
	uint32_t __v;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0":"=r"(__v));
    __v &= ~val;
    asm volatile ("mcr p15, 0, %0, c1, c0, 0"::"r"(__v));
}

/* C1 (C)oprocessor (A)ccess (C)ontrol (R)egister SET ALL */
static inline void CP15_C1CACR_SetAll(uint32_t val) {
	uint32_t __v;
    asm volatile ("mrc p15, 0, %0, c1, c0, 2":"=r"(__v));
    __v |= (C1CACRF_CPAP & val);
    asm volatile ("mcr p15, 0, %0, c1, c0, 2"::"r"(__v));
	asm volatile ("isb");
}

/* C1 (C)oprocessor (A)ccess (C)ontrol (R)egister SET NONE */
static inline void CP15_C1CACR_SetNone(uint32_t val) {
	uint32_t __v;
    asm volatile ("mrc p15, 0, %0, c1, c0, 2":"=r"(__v));
	__v &= (C1CACRF_CPAP & ~val);
    asm volatile ("mcr p15, 0, %0, c1, c0, 2"::"r"(__v));
	asm volatile ("isb");
}

#endif /* ASM_ARM_CP15_H */

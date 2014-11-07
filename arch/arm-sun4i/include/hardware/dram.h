/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i dram control module
    Lang: english
*/

#ifndef HARDWARE_SUN4I_DRAM_H
#define HARDWARE_SUN4I_DRAM_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4I_DRAM_BASE  0x01c01000

#define DRAM_CCR         (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0000))
#define DRAM_DCR         (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0004))
#define DRAM_IOCR        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0008))
#define DRAM_CSR         (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x000c))
#define DRAM_DRR         (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0010))
#define DRAM_TPR0        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0014))
#define DRAM_TPR1        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0018))
#define DRAM_TPR2        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x001c))
#define DRAM_GDLLCR      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0020))
#define DRAM_RSLR0       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x004c))
#define DRAM_RSLR1       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0050))
#define DRAM_RDGR0       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x005c))
#define DRAM_RDGR1       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0060))
#define DRAM_ODTCR       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0098))
#define DRAM_DTR0        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x009c))
#define DRAM_DTR1        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x00a0))
#define DRAM_DTAR        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x00a4))
#define DRAM_ZQCR0       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x00a8))
#define DRAM_ZQCR1       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x00ac))
#define DRAM_ZQSR        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x00b0))
#define DRAM_IDCR        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x00b4))
#define DRAM_MR          (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x01f0))
#define DRAM_EMR         (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x01f4))
#define DRAM_EMR2        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x01f8))
#define DRAM_EMR3        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x01fc))
#define DRAM_DLLCTR      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0200))
#define DRAM_DLLCR       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0204))
#define DRAM_DLLCR0      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0204))
#define DRAM_DLLCR1      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0208))
#define DRAM_DLLCR2      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x020c))
#define DRAM_DLLCR3      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0210))
#define DRAM_DLLCR4      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0214))
#define DRAM_DQTR0       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0218))
#define DRAM_DQTR1       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x021c))
#define DRAM_DQTR2       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0220))
#define DRAM_DQTR3       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0224))
#define DRAM_DQSTR       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0228))
#define DRAM_DQSBTR      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x022c))
#define DRAM_MCR         (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0230))
#define DRAM_PPWRSCTL    (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x023c))
#define DRAM_APR         (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0240))
#define DRAM_PLDTR       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0244))
#define DRAM_HPCR        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0250))
#define DRAM_HPCR0       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0250))
#define DRAM_HPCR1       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0254))
#define DRAM_HPCR2       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0258))
#define DRAM_HPCR3       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x025c))
#define DRAM_HPCR4       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0260))
#define DRAM_HPCR5       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0264))
#define DRAM_HPCR6       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0268))
#define DRAM_HPCR7       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x026c))
#define DRAM_HPCR8       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0270))
#define DRAM_HPCR9       (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0274))
#define DRAM_HPCR10      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0278))
#define DRAM_HPCR11      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x027c))
#define DRAM_HPCR12      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0280))
#define DRAM_HPCR13      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0284))
#define DRAM_HPCR14      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0288))
#define DRAM_HPCR15      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x028c))
#define DRAM_HPCR16      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0290))
#define DRAM_HPCR17      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0294))
#define DRAM_HPCR18      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x0298))
#define DRAM_HPCR19      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x029c))
#define DRAM_HPCR20      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02a0))
#define DRAM_HPCR21      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02a4))
#define DRAM_HPCR22      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02a8))
#define DRAM_HPCR23      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02ac))
#define DRAM_HPCR24      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02b0))
#define DRAM_HPCR25      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02b4))
#define DRAM_HPCR26      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02b8))
#define DRAM_HPCR27      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02bc))
#define DRAM_HPCR28      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02c0))
#define DRAM_HPCR29      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02c4))
#define DRAM_HPCR30      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02c8))
#define DRAM_HPCR31      (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02cc))
#define DRAM_CSEL        (*(volatile uint32_t *)(SUN4I_DRAM_BASE + 0x02e0))

#define SUN4I_DRAM_MAGIC1 0x16237495

#endif

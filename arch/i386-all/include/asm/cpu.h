#ifndef ASM_I386_CPU_H
#define ASM_I386_CPU_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: assembler-level specific definitions for x86 CPU
    Lang: english
*/

#include <inttypes.h>

/* This file is very very incomplete :) */

#define HALT asm volatile("hlt")

/* Table descriptor used for lgdt and lidt commands */
struct table_desc
{
    unsigned short size;
    unsigned long  base;
} __attribute__((packed));

/* Segment descriptor in the GDT */
struct segment_desc
{
    uint16_t    limit_low;
    uint16_t    base_low;
    uint8_t     base_mid;
    unsigned    type:5, dpl:2, p:1;
    unsigned    limit_high:4, avl:1, l:1, d:1, g:1;
    uint8_t     base_high:8;
} __attribute__((packed));

/*
 * TaskStateStructure, defined only in matter of making life (setup)
 * more human-readable
 */
struct tss
{
    unsigned int   link,               /* link to previous task         */
                   ssp,                /* Supervisor Stack Pointer      */
                   ssp_seg,            /* SSP descriptor                */
                   t0,t1,              /* Stack for CPL1 code           */
                   t2,t3,              /* Stack for CPL2 code           */
                   cr3,                /* used in paging                */
                   eip,                /* Instruction pointer           */
                   eflags,             /* Flags for given task          */
                   r0,r1,r2,r3,        /* 8 general purpouse registers  */
                   r4,r5,r6,r7,
                   es,cs,ss,ds,fs,gs,  /* segment descriptors           */
                   ldt;                /* LocalDescriptorTable          */
    unsigned short trap,iomap;         /* trap flag and iomap pointer   */
};

#define rdcr(reg) \
    ({ long val; asm volatile("mov %%" #reg ",%0":"=r"(val)); val; })

#define wrcr(reg, val) \
    do { asm volatile("mov %0,%%" #reg::"r"(val)); } while(0)

static inline void __attribute__((always_inline)) rdmsr(uint32_t msr_no, uint32_t *ret_lo, uint32_t *ret_hi)
{
    uint32_t ret1, ret2;

    asm volatile("rdmsr":"=a"(ret1),"=d"(ret2):"c"(msr_no));
    *ret_lo=ret1;
    *ret_hi=ret2;
}

static inline uint32_t __attribute__((always_inline)) rdmsri(uint32_t msr_no)
{
    uint32_t ret;

    asm volatile("rdmsr":"=a"(ret):"c"(msr_no));
    return ret;
}

#endif

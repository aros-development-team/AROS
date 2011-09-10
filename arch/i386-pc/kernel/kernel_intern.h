/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: kernel_intern.h
    Lang: english
*/

#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <asm/cpu.h>
#include <inttypes.h>

#define STACK_SIZE 8192

struct PlatformData
{
    long long  *idt;
    struct tss *tss;
    uint16_t    xtpic_mask;
};

extern struct segment_desc *GDT;

void core_Unused_Int(void);

#endif /* KERNEL_INTERN_H_ */

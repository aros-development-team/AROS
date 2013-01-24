/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <aros/libcall.h>
#include <inttypes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <stdio.h>
#include <stdarg.h>

#include <asm/bcm2835.h>

#undef KernelBase

/*
// Enable support for paging memory to media..
#define RASPI_VIRTMEMSUPPORT
*/

/* Use system timer 3 for our scheduling heartbeat */
#define VBLANK_TIMER            3
#define VBLANK_INTERVAL         (1000000 / 50)

#define KERNEL_PHYS_BASE        0x07800000
#define KERNEL_VIRT_BASE        0xff800000

#define gpioGPSET0 ((volatile unsigned int *)(GPSET0))
#define gpioGPCLR0 ((volatile unsigned int *)(GPCLR0))

void core_SetupMMU(void);
void core_SetupIntr(void);

void *KrnAddSysTimerHandler(struct KernelBase *);

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);

struct KernelBase *getKernelBase();

#ifdef bug
#undef bug
#endif
#ifdef D
#undef D
#endif
#define D(x) x

#define __STR(x) #x
#define STR(x) __STR(x)

AROS_LD2(int, KrnBug,
         AROS_LDA(const char *, format, A0),
         AROS_LDA(va_list, args, A1),
         struct KernelBase *, KernelBase, 12, Kernel);

static inline void bug(const char *format, ...)
{
    struct KernelBase *kbase = getKernelBase();
    va_list args;
    va_start(args, format);
    AROS_SLIB_ENTRY(KrnBug, Kernel, 12)(format, args, kbase);
    va_end(args);
}

#endif /*KERNEL_INTERN_H_*/

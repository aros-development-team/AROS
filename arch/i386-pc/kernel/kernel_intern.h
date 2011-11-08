/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: kernel_intern.h
    Lang: english
*/

#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <asm/cpu.h>
#include <hardware/vbe.h>
#include <resources/acpi.h>

#include <inttypes.h>

#define STACK_SIZE 8192
#define PAGE_SIZE  0x1000

struct PlatformData
{
    long long	    *idt;
    struct tss	    *tss;
    uint16_t	     xtpic_mask;
    APTR	     kb_APIC_TrampolineBase;
    struct APICData *kb_APIC;
};

extern struct segment_desc *GDT;

void vesahack_Init(char *cmdline, struct vbe_mode *vmode);
void core_Unused_Int(void);
void PlatformPostInit(void);
int smp_Setup(void);
int smp_Wake(void);

struct ExceptionContext;

void core_LeaveInterrupt(struct ExceptionContext *);
void core_Supervisor(struct ExceptionContext *);
void core_Reboot(void);

#endif /* KERNEL_INTERN_H_ */

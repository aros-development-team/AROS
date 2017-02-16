#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_
/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 32bit x86 kernel_intern.h
    Lang: english
*/

#include <asm/cpu.h>
#include <hardware/vbe.h>

#include <inttypes.h>

typedef struct int_gate_32bit apicidt_t;

#include "apic.h"

#define STACK_SIZE 8192
#define PAGE_SIZE	0x1000
#define PAGE_MASK	0x0FFF

/*
 * Boot-time private data.
 * This structure is write-protected in user mode and survives warm restarts.
 */
struct KernBootPrivate
{
    IPTR		        _APICBase;		/* Bootstrap APIC base address				*/
    UWORD                       kbp_APIC_BSPID;		/* Bootstrap APIC logical ID				*/
    APTR                        BOOTGDT,
                                BOOTIDT;
    void	                *TSS;
    struct CPUMMUConfig         MMU;
};

extern struct KernBootPrivate *__KernBootPrivate;

/* Platform-specific part of KernelBase */
struct PlatformData
{
    struct List         kb_SysCallHandlers;
    APTR	     kb_APIC_TrampolineBase;
    struct ACPIData *kb_ACPI;
    struct APICData *kb_APIC;
    struct IOAPICData   *kb_IOAPIC;
};

#define IDT_SIZE                sizeof(apicidt_t) * 256
#define GDT_SIZE                sizeof(long long) * 8
#define TLS_SIZE                sizeof(struct tss)
#define TLS_ALIGN               64

#define __save_flags(x)		__asm__ __volatile__("pushf ; pop %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("push %0 ; popf": /* no output */ :"g" (x):"memory", "cc")

#define krnLeaveSupervisorRing(_flags)                          \
    asm("movl %[user_ds],%%eax\n\t"                             \
        "mov %%eax,%%ds\n\t"                                    \
	"mov %%eax,%%es\n\t"                                    \
	"movl %%esp,%%ebx\n\t"                                  \
	"pushl %%eax\n\t"                                       \
	"pushl %%ebx\n\t"                                       \
	"pushl %[iflags]\n\t"                                   \
	"pushl %[cs]\n\t"                                       \
	"pushl $1f\n\t"                                         \
	"iret\n"                                                \
	"1:"                                                    \
        : : [user_ds] "r" (USER_DS), [cs] "i" (USER_CS),        \
            [iflags] "i" (_flags)                               \
        :"eax","ebx")

#define FLAGS_INTENABLED        0x3002

void vesahack_Init(char *cmdline, struct vbe_mode *vmode);
void core_Unused_Int(void);

/** CPU Functions **/
#if (0)
void core_SetupGDT(struct KernBootPrivate *, apicid_t, APTR, APTR, APTR);
void core_SetupMMU(struct KernBootPrivate *, IPTR memtop);
#endif

void core_CPUSetup(apicid_t, APTR, IPTR);

void ictl_Initialize(struct KernelBase *KernelBase);


/* HW IRQ Related Functions */
struct ExceptionContext;
extern const void *IntrDefaultGates[256];

void core_LeaveInterrupt(struct ExceptionContext *);
void core_Supervisor(struct ExceptionContext *);

void PlatformPostInit(void);

#endif /* KERNEL_INTERN_H_ */

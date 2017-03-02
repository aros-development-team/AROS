#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_
/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 64bit x86 kernel_intern.h
    Lang: english
*/

#include <inttypes.h>

#include <aros/kernel.h>
#include <utility/tagitem.h>
#include <asm/cpu.h>

typedef struct int_gate_64bit apicidt_t;

#include "apic.h"
#include "tls.h"

#define STACK_SIZE              65536

#define DEF_IRQRETFUNC          core_DefaultIRETQ

struct ACPIData;
struct IOAPICData;

/*
 * Boot-time private data.
 * This structure is write-protected in user mode and survives warm restarts.
 */
struct KernBootPrivate
{
    IPTR		        _APICBase;		/* Bootstrap APIC base address				*/
    UWORD                       kbp_APIC_BSPID;		/* Bootstrap APIC logical ID				*/
    unsigned short	        debug_y_resolution;	/* Parameters of screen's lower half ('vesahack' mode)	*/
    void	                *debug_framebuffer;
    IPTR	                SystemStack;		/* System stack base address	       			*/
    APTR                        BOOTTLS,
                                BOOTGDT,
                                BOOTIDT;
    void	                *TSS;
    struct CPUMMUConfig         MMU;
};

extern struct KernBootPrivate *__KernBootPrivate;

#define IDT_SIZE                sizeof(struct int_gate_64bit) * 256
#define GDT_SIZE                sizeof(struct gdt_64bit) + 128
#define TLS_SIZE                sizeof(tls_t)
#define TLS_ALIGN               sizeof(APTR)

#define __save_flags(x)		__asm__ __volatile__("pushfq ; popq %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("pushq %0 ; popfq": /* no output */ :"g" (x):"memory", "cc")

#define krnLeaveSupervisorRing(_flags)                          \
    asm volatile (                                              \
            "mov %[user_ds],%%ds\n\t"                           \
            "mov %[user_ds],%%es\n\t"                           \
            "mov %%rsp,%%r12\n\t"                               \
            "pushq %[ds]\n\t"      	                        \
            "pushq %%r12\n\t"                                   \
            "pushq %[iflags]\n\t"                               \
            "pushq %[cs]\n\t"		                        \
            "pushq $1f\n\t"                                     \
            "iretq\n 1:"                                        \
            : : [user_ds] "r" (USER_DS), [ds] "i" (USER_DS),    \
                [cs] "i" (USER_CS), [iflags] "i" (_flags)       \
            : "r12")

#define FLAGS_INTENABLED        0x3002

/* Main boot code */
void core_Kick(struct TagItem *msg, void *target);
void kernel_cstart(const struct TagItem *msg);
void PlatformPostInit(void);

/** CPU Related Functions **/
void core_SetupGDT(struct KernBootPrivate *, apicid_t, APTR, APTR, APTR);

void core_SetupMMU(struct CPUMMUConfig *, IPTR memtop);
void core_InitMMU(struct CPUMMUConfig *);
void core_LoadMMU(struct CPUMMUConfig *);

void core_CPUSetup(apicid_t, APTR, IPTR);

/* */
void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us);
void core_Supervisor(struct ExceptionContext *);

/* HW IRQ Related Functions */
struct ExceptionContext;
extern const void *IntrDefaultGates[256];

void ictl_Initialize(struct KernelBase *KernelBase);

void core_LeaveInterrupt(struct ExceptionContext *);

#endif /*KERNEL_INTERN_H_*/

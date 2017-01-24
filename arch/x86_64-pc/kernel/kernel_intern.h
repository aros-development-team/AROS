#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>

#include <exec/nodes.h>
#include <exec/lists.h>
#include <aros/kernel.h>
#include <utility/tagitem.h>
#include <asm/cpu.h>

#include "apic.h"

#define STACK_SIZE      65536
#define PAGE_SIZE	0x1000
#define PAGE_MASK	0x0FFF

struct ACPIData;
struct IOAPICData;

/*
 * Boot-time private data.
 * This structure is write-protected in user mode and survives warm restarts.
 */
struct KernBootPrivate
{
    IPTR		_APICBase;		/* Bootstrap APIC base address				*/
    UWORD               kbp_APIC_BSPID;		/* Bootstrap APIC logical ID				*/
    unsigned short	debug_y_resolution;	/* Parameters of screen's lower half ('vesahack' mode)	*/
    void	       *debug_framebuffer;
    IPTR	        SystemStack;		/* System stack base address	       			*/
    APTR                BOOTTLS,
                        BOOTGDT,
                        BOOTIDT;
    void	       *TSS;
    void	       *PML4;
    void	       *PDP;
    void	       *PDE;
    void	       *PTE;
    int		        used_page;
};

extern struct KernBootPrivate *__KernBootPrivate;

/* Platform-specific part of KernelBase */
struct PlatformData
{
    APTR                kb_APIC_TrampolineBase;	/* Starting address of secondary core bootstrap code	*/
    uint16_t            kb_XTPIC_Mask;		/* Current XT-PIC interrupt mask			*/
    struct ACPIData     *kb_ACPI;
    struct APICData     *kb_APIC;
    struct IOAPICData   *kb_IOAPIC;
};

#define KBL_INTERNAL    0
#define KBL_XTPIC       1
#define KBL_APIC        2
#define KBL_IOAPIC      3

#if (0)
#define IDT_GET()                __KernBootPrivate->IDT
#define IDT_SET(val) \
    do { \
        __KernBootPrivate->IDT = val; \
    } while(0);
#else
#define IDT_GET()               TLS_GET(IDT)
#define IDT_SET(val)            TLS_SET(IDT, val);
#define GDT_GET()               TLS_GET(GDT)
#define GDT_SET(val)            TLS_SET(GDT, val);
#endif

/* Main boot code */
void core_Kick(struct TagItem *msg, void *target);
void kernel_cstart(const struct TagItem *msg);

/** CPU Functions **/
void core_SetupIDT(struct KernBootPrivate *, apicid_t, APTR);
void core_SetupGDT(struct KernBootPrivate *, apicid_t, APTR, APTR, APTR);
void core_SetupMMU(struct KernBootPrivate *, IPTR memtop);

void core_CPUSetup(apicid_t, APTR, IPTR);

void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us);
void core_DefaultIRETQ();
void ictl_Initialize(void);

struct ExceptionContext;

/* Interrupt processing */
void core_LeaveInterrupt(struct ExceptionContext *);
void core_Supervisor(struct ExceptionContext *);

//void core_Reboot(void);

void PlatformPostInit(void);

//int smp_Setup(void);
//int smp_Wake(void);

#endif /*KERNEL_INTERN_H_*/

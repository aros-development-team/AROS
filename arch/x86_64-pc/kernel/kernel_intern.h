#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>

#include <exec/nodes.h>
#include <exec/lists.h>
#include <aros/kernel.h>
#include <utility/tagitem.h>
#include <asm/cpu.h>

#define STACK_SIZE      65536
#define PAGE_SIZE	0x1000
#define PAGE_MASK	0x0FFF

/*
 * Boot-time private data.
 * This structure is write-protected in user mode and survives warm restarts.
 */
struct KernBootPrivate
{
    IPTR	        kbp_PrivateNext;	/* Boot-time memory allocation pointer 			*/
    const struct GenericAPIC *kbp_APIC_Driver;	/* Probed APIC driver		       			*/
    IPTR		_APICBase;		/* Bootstrap APIC base address				*/
    UWORD               kbp_APIC_BSPID;		/* Bootstrap APIC logical ID				*/
    unsigned short	debug_y_resolution;	/* Parameters of screen's lower half ('vesahack' mode)	*/
    void	       *debug_framebuffer;
    IPTR	        SystemStack;		/* System stack base address	       			*/
    void	       *GDT;			/* Self-explanatory					*/
    void	       *TSS;
    void	       *system_tls;
    void	       *IDT;
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
    uint16_t            kb_XTPIC_Mask;
    UBYTE               kb_APIC_Count;		/* How many APICs (CPU cores) we have			*/
    UWORD               *kb_APIC_IDMap;         /* ACPI_ID << 8 | LOGICAL_ID	      			*/
    IPTR                *kb_APIC_BaseMap;
    int                 kb_APIC_IRQ_Model;
    int                 kb_ACPI_IOAPIC;
};

#define KBL_INTERNAL    0
#define KBL_XTPIC       1
#define KBL_APIC        2

/* Main boot code */
void core_Kick(struct TagItem *msg, void *target);
void kernel_cstart(const struct TagItem *msg);

/** CPU Functions **/
IPTR core_APICProbe(struct KernBootPrivate *);
UBYTE core_APICGetTotal();
UBYTE core_APICGetNumber();
void core_SetupIDT(struct KernBootPrivate *);
void core_SetupGDT(struct KernBootPrivate *);
void core_SetupMMU(struct KernBootPrivate *);
void core_CPUSetup(UBYTE, IPTR);
void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us);
void core_DefaultIRETQ();

struct ExceptionContext;

/* Interrupt processing */
void core_LeaveInterrupt(struct ExceptionContext *);
void core_Supervisor(struct ExceptionContext *);

#endif /*KERNEL_INTERN_H_*/

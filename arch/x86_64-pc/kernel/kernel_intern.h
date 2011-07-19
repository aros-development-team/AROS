#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>

#include <exec/nodes.h>
#include <exec/lists.h>
#include <aros/kernel.h>
#include <utility/tagitem.h>
#include <asm/cpu.h>

#define STACK_SIZE      8192
#define PAGE_SIZE	0x1000
#define PAGE_MASK	0x0FFF

struct   KernBootPrivate
{
    IPTR	        kbp_PrivateNext;
    IPTR                kbp_InitFlags;
    IPTR                kbp_ACPIRSDP;
    struct MemHeader   *kbp_LowMem;
    const struct GenericAPIC **kbp_APIC_Drivers;
    IPTR                kbp_APIC_DriverID;
    UWORD               kbp_APIC_BSPID;
    int                 kbp_APIC_IRQ_Model;
    unsigned short	debug_y_resolution;
    void	       *debug_framebuffer;
};

#define KERNBOOTFLAG_SERDEBUGCONFIGURED (1 << 0)
#define KERNBOOTFLAG_DEBUG              (1 << 1)
#define KERNBOOTFLAG_BOOTCPUSET         (1 << 2)

extern struct KernBootPrivate *__KernBootPrivate;
extern struct PML4E PML4[512];

/* Platform-specific part of KernelBase */
struct PlatformData
{
    IPTR                kb_ACPIRSDP;
    APTR                kb_APIC_TrampolineBase;
    const struct GenericAPIC **kb_APIC_Drivers;
    IPTR                kb_APIC_DriverID;
    uint16_t            kb_XTPIC_Mask;
    UBYTE               kb_APIC_Count;
    UBYTE		kb_APIC_Ready;
    UWORD               *kb_APIC_IDMap;                         /* ACPI_ID << 8 | LOGICAL_ID */
    IPTR                *kb_APIC_BaseMap;
    int                 kb_APIC_IRQ_Model;
    int                 kb_ACPI_IOAPIC;
    int                 kb_SMP_Config;
};

#define KBL_INTERNAL    0
#define KBL_XTPIC       1
#define KBL_APIC        2

/** CPU Functions **/
IPTR core_APICProbe(struct KernBootPrivate *);
UBYTE core_APICGetTotal();
UBYTE core_APICGetNumber();
void core_SetupIDT(struct KernBootPrivate *);
void core_SetupGDT(struct KernBootPrivate *);
void core_SetupMMU(struct KernBootPrivate *);
void core_CPUSetup(IPTR);
void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us);
void core_DefaultIRETQ();

struct ExceptionContext;

/* Interrupt processing */
void core_LeaveInterrupt(struct ExceptionContext *);
void core_Supervisor(struct ExceptionContext *);

#endif /*KERNEL_INTERN_H_*/

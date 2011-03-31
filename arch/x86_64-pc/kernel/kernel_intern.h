#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <inttypes.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <aros/kernel.h>
#include <utility/tagitem.h>
#include <asm/cpu.h>

#include <stdio.h>

#include "acpi.h"
#include "apic.h"

struct   KernBootPrivate
{
    IPTR	        kbp_PrivateNext;
    IPTR                kbp_InitFlags;
    IPTR                kbp_ACPIRSDP;
    struct MemHeader   *kbp_LowMem;
    const struct GenericAPIC  **kbp_APIC_Drivers;
    IPTR                kbp_APIC_DriverID;
    UWORD               kbp_APIC_BSPID;
    int                 kbp_APIC_IRQ_Model;
};

#define KERNBOOTFLAG_SERDEBUGCONFIGURED (1 << 0)
#define KERNBOOTFLAG_DEBUG              (1 << 1)
#define KERNBOOTFLAG_BOOTCPUSET         (1 << 2)

extern struct KernBootPrivate *__KernBootPrivate;
extern struct PML4E PML4[512];

struct KernelBase
{
    struct Node         kb_Node;
    struct List         kb_Intr[256];

    IPTR                kb_ACPIRSDP;

    APTR                kb_APIC_TrampolineBase;
    const struct GenericAPIC  **kb_APIC_Drivers;
    IPTR                kb_APIC_DriverID;
    uint16_t            kb_XTPIC_Mask;
    UBYTE               kb_APIC_Count;
    UWORD               *kb_APIC_IDMap;                         /* ACPI_ID << 8 | LOGICAL_ID */
    IPTR                *kb_APIC_BaseMap;
    int                 kb_APIC_IRQ_Model;
};

#define KBL_INTERNAL    0
#define KBL_XTPIC       1
#define KBL_APIC        2

struct IntrNode {
    struct MinNode      in_Node;
    void                (*in_Handler)(void *, void *);
    void                *in_HandlerData;
    void                *in_HandlerData2;
};

#define SC_CAUSE        0
#define SC_DISPATCH     1
#define SC_SWITCH       2
#define SC_SCHEDULE     3

void core_LeaveInterrupt(regs_t *regs) __attribute__((noreturn));
void core_Switch(regs_t *regs) __attribute__((noreturn));
void core_Schedule(regs_t *regs) __attribute__((noreturn));
void core_Dispatch(regs_t *regs) __attribute__((noreturn));
void core_ExitInterrupt(regs_t *regs) __attribute__((noreturn)); 
void core_IRQHandle(regs_t regs);
void core_Cause(struct ExecBase *SysBase);
/** ACPI Functions **/
IPTR core_ACPIProbe(struct TagItem *, struct KernBootPrivate *);
ULONG core_ACPIInitialise(struct KernelBase *);
int core_ACPIIsBlacklisted();
IPTR core_ACPIRootSystemDescriptionPointerLocate();
IPTR core_ACPIRootSystemDescriptionPointerScan(IPTR, IPTR);
int core_ACPITableChecksum(void *, unsigned long);
IPTR core_ACPITableSDTGet(struct acpi_table_rsdp *);
int core_ACPITableParse(int, struct acpi_table_hook *);
int core_ACPITableMADTParse(int, struct acpi_madt_entry_hook *);
int core_ACPITableMADTFamParse(int, unsigned long, int, struct acpi_madt_entry_hook *);
int core_ACPITableHeaderEarly(int, struct acpi_table_header **);
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

/* Silly renaming, all rkprintf() calls need to be replaced with bug() */
#undef bug
#undef rkprintf
#define rkprintf bug

int bug(const char *format, ...);

#endif /*KERNEL_INTERN_H_*/

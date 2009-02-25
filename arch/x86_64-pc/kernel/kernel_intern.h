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
    IPTR                kbp_PrivateNext;
    IPTR                kbp_InitFlags;
    IPTR                kbp_ACPIRSDP;
    IPTR                kbp_APIC_TrampolineBase;
    const struct GenericAPIC  **kbp_APIC_Drivers;
    IPTR                kbp_APIC_DriverID;
    UWORD               kbp_APIC_BSPID;
    int                 kbp_APIC_IRQ_Model;
    char                kbp_BOOTCmdLine[200];
};

#define KERNBOOTFLAG_SERDEBUGCONFIGURED (1 << 0)
#define KERNBOOTFLAG_DEBUG              (1 << 1)
#define KERNBOOTFLAG_BOOTCPUSET         (1 << 2)

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Intr[256];

    IPTR                kb_ACPIRSDP;

    IPTR                kb_APIC_TrampolineBase;
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

int exec_main(struct TagItem *msg, void *entry);
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
UBYTE core_APICGetNumber();
void core_SetupIDT(struct KernBootPrivate *);
void core_SetupGDT(struct KernBootPrivate *);
void core_SetupMMU(struct KernBootPrivate *);
void core_CPUSetup(IPTR);
void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us);
void core_DefaultIRETQ();
/** Kernel Attribute Functions **/
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
void krnSetTagData(Tag tagValue, intptr_t newtagValue, const struct TagItem *tagList);

/* Debug support .. */
extern void Exec_SerialRawIOInit();
extern void scr_RawPutChars(char *, int);
extern ULONG            __serial_rawio_speed;
extern UBYTE            __serial_rawio_databits;
extern UBYTE            __serial_rawio_parity;
extern UBYTE            __serial_rawio_stopbits;
extern UWORD            __serial_rawio_port;
extern unsigned char    __serial_rawio_debug;

void clr();
static char tab[512];
#ifdef rkprintf
#undef rkprintf
#endif
#define rkprintf(x...) scr_RawPutChars(tab, snprintf(tab, 510, x))

#endif /*KERNEL_INTERN_H_*/

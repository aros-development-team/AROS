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

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Intr[256];
    IPTR                kb_ACPIRSDP;
    uint16_t            kb_XTPIC_Mask;
    IPTR                kb_APICBase;
    UBYTE               *kb_APICIDMap;
    UBYTE               kb_APICCount;
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

#define STACK_SIZE      8192
#define PAGE_SIZE	    0x1000

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
IPTR core_ACPIProbe();
ULONG core_ACPIInitialise();
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
IPTR core_APICProbe();
IPTR core_APICGetMSRAPICBase();
UBYTE core_APICGetID(IPTR);
void core_APICInitialise(IPTR);
unsigned long core_APICIPIWake(UBYTE, IPTR);
void core_SetupIDT();
void core_SetupGDT();
void core_SetupMMU();
void core_CPUSetup(IPTR);
void core_ProtKernelArea(intptr_t addr, intptr_t length, char p, char rw, char us);
void core_DefaultIRETQ();
/** Kernel Attribute Functions **/
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
void krnSetTagData(Tag tagValue, intptr_t newtagValue, const struct TagItem *tagList);

/* Debug support .. */
void scr_RawPutChars(char *, int);
void clr();
static char tab[512];
#ifdef rkprintf
#undef rkprintf
#endif
#define rkprintf(x...) scr_RawPutChars(tab, snprintf(tab, 510, x))

#endif /*KERNEL_INTERN_H_*/

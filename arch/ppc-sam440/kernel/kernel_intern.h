#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <aros/libcall.h>
#include <inttypes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <asm/amcc440.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(DEBUG) && DEBUG
#include <aros/debug.h>
struct KernelBase;
#include "kernel_debug.h"

/* Early definition of 'bug'
 */
#undef bug
static inline void bug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    krnBug(fmt, args, NULL);
    va_end(args);
}
#endif

#define KERNEL_PHYS_BASE        0x00800000
#define KERNEL_VIRT_BASE        0xff800000

#define STACK_SIZE 4096

struct PlatformData {
    struct MemHeader *  pd_SupervisorMem;
    struct MinList      pd_Modules;
    context_t *         pd_FPUOwner;
    uint32_t            pd_CPUUsage;

    uint32_t            pd_CPUFreq;
    uint32_t            pd_PLBFreq;
    uint32_t            pd_OPBFreq;
    uint32_t            pd_PCIFreq;
    uint32_t            pd_EPBFreq;

    uint32_t            pd_PVR;         /* Cache of the PVR SPR */
};

static inline BOOL krnIsPPC440(uint32_t pvr)
{
    return (pvr == PVR_PPC440EP_B || pvr == PVR_PPC440EP_C);
}

static inline BOOL krnIsPPC460(uint32_t pvr)
{
    return (pvr == PVR_PPC460EX_B);
}

enum intr_types {
    it_exception = 0xe0,
    it_interrupt = 0xf0
};

struct IntrNode {
    struct MinNode      in_Node;
    void                (*in_Handler)(void *, void *);
    void                *in_HandlerData;
    void                *in_HandlerData2;
    uint8_t             in_type;
    uint8_t             in_nr;
};

typedef struct {
        struct MinNode  m_node;
        char *          m_name;
        char *          m_str;
        intptr_t        m_lowest;
        intptr_t        m_highest;
        struct MinList  m_symbols;
} module_t;

typedef struct {
        struct MinNode  s_node;
        char *                  s_name;
        intptr_t        s_lowest;
        intptr_t        s_highest;
} symbol_t;

static inline struct ExecBase *getSysBase(void)
{
    return (struct ExecBase *)rdspr(SPRG5U);
}

static inline uint64_t mftbu()
{
        uint32_t lo,hi,tmp;

        do {
                asm volatile("mftbu %0; mftb %1; mftbu %2":"=r"(hi),"=r"(lo),"=r"(tmp));
        } while(tmp != hi);

        return (((uint64_t)hi) << 32) | ((uint64_t)lo);
}

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, struct TagItem *tagList);
struct TagItem *krnNextTagItem(struct TagItem **tagListPtr);

void core_LeaveInterrupt(context_t *ctx);

void mmu_init(struct TagItem *tags);
void intr_init();

void syscall_handler(context_t *ctx, uint8_t exception, void *self);
void uic_handler(context_t *ctx, uint8_t exception, void *self);

uint32_t findNames(intptr_t addr, char **module, char **function);

#endif /*KERNEL_INTERN_H_*/

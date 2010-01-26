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

#include "syscall.h"

#define KERNEL_PHYS_BASE        0x00800000
#define KERNEL_VIRT_BASE        0xff800000

#define STACK_SIZE 4096

struct KernelBase {
    struct Node         kb_Node;
    struct List         kb_Exceptions[16];
    struct List         kb_Interrupts[64];
    struct MemHeader *  kb_SupervisorMem;
    struct MinList      kb_Modules;
    context_t *			kb_FPUOwner;
    uint32_t			kb_CPUUsage;

    uint32_t			kb_CPUFreq;
    uint32_t			kb_PLBFreq;
    uint32_t			kb_OPBFreq;
    uint32_t			kb_PCIFreq;
    uint32_t			kb_EPBFreq;
};

struct KernelBSS {
    void *addr;
    uint32_t len;
};

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
	struct MinNode	m_node;
	char *          m_name;
	char *          m_str;
	intptr_t        m_lowest;
	intptr_t        m_highest;
	struct MinList  m_symbols;
} module_t;

typedef struct {
	struct MinNode  s_node;
	char *			s_name;
	intptr_t        s_lowest;
	intptr_t        s_highest;
} symbol_t;

static inline struct KernelBase *getKernelBase()
{
    return (struct KernelBase *)rdspr(SPRG4U);
}

static inline struct ExecBase *getSysBase()
{
    return (struct ExecBase *)rdspr(SPRG5U);
}

static inline uint32_t goSuper() {
	register uint32_t oldmsr asm("r3");
	asm volatile("li %0,%1; sc":"=r"(oldmsr):"i"(SC_SUPERSTATE):"memory");
	return oldmsr;
}

static inline void goUser() {
    wrmsr(rdmsr() | (MSR_PR));
}

static inline uint64_t mftbu()
{
	uint32_t lo,hi,tmp;

	do {
		asm volatile("mftbu %0; mftb %1; mftbu %2":"=r"(hi),"=r"(lo),"=r"(tmp));
	} while(tmp != hi);

	return (((uint64_t)hi) << 32) | ((uint64_t)lo);
}

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);

void core_LeaveInterrupt(regs_t *regs) __attribute__((noreturn));
void core_Switch(regs_t *regs) __attribute__((noreturn));
void core_Schedule(regs_t *regs) __attribute__((noreturn));
void core_Dispatch(regs_t *regs) __attribute__((noreturn));
void core_ExitInterrupt(regs_t *regs) __attribute__((noreturn)); 
void core_Cause(struct ExecBase *SysBase);
void mmu_init(struct TagItem *tags);
void intr_init();

void __attribute__((noreturn)) syscall_handler(regs_t *ctx, uint8_t exception, void *self);
void __attribute__((noreturn)) uic_handler(regs_t *ctx, uint8_t exception, void *self);

#ifdef bug
#undef bug
#endif
#ifdef D
#undef D
#endif
#define D(x) x

AROS_LD2(int, KrnBug,
         AROS_LDA(const char *, format, A0),
         AROS_LDA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel);

static inline void bug(const char *format, ...)
{
    struct KernelBase *kbase = getKernelBase();
    va_list args;
    va_start(args, format);
    AROS_SLIB_ENTRY(KrnBug, Kernel)(format, args, kbase);
    va_end(args);
}

uint32_t findNames(intptr_t addr, char **module, char **function);

#endif /*KERNEL_INTERN_H_*/

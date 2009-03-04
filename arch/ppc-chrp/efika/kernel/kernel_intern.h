#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <asm/mpc5200b.h>
#include <aros/libcall.h>
#include <inttypes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <stdio.h>
#include <stdarg.h>

#include "syscall.h"

#define KERNEL_PHYS_BASE        0x07800000
#define KERNEL_VIRT_BASE        0xff800000

#define STACK_SIZE 4096

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Exceptions[21];
    struct List         kb_Interrupts[64];
    struct MemHeader    *kb_SupervisorMem;
    struct MinList		kb_Modules;
    context_t			*kb_FPUOwner;
};

struct OFWNode {
	struct MinNode	on_node;
	char 			*on_name;
	struct MinList	on_children;
	struct MinList	on_properties;

	uint8_t	on_storage[];
};

struct OFWProperty {
	struct MinNode	op_node;
	char 			*op_name;
	uint32_t		op_length;
	void			*op_value;

	uint8_t			op_storage[];
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

/*
 * Exception handler differs a bit. It is suppost to have access to the CPU
 * context...
 */
struct ExceptNode {
    struct MinNode      in_Node;
    int                 (*in_Handler)(regs_t *, void *, void *);
    void                *in_HandlerData;
    void                *in_HandlerData2;
    uint8_t             in_type;
    uint8_t             in_nr;
};

typedef struct {
	struct MinNode 	m_node;
	char			*m_name;
	char			*m_str;
	intptr_t		m_lowest;
	intptr_t		m_highest;
	struct MinList	m_symbols;
} module_t;

typedef struct {
	struct MinNode	s_node;
	char			*s_name;
	intptr_t		s_lowest;
	intptr_t		s_highest;
} symbol_t;

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
void intr_init();
void mmu_init(char *mmu_dir, uint32_t mmu_size);
void ictl_init(void *);
void ictl_enable_irq(uint8_t irqnum);
void ictl_disable_irq(uint8_t irqnum);
void __attribute__((noreturn)) syscall_handler(regs_t *ctx, uint8_t exception, void *self);
void __attribute__((noreturn)) ictl_handler(regs_t *ctx, uint8_t exception, void *self);
void __attribute__((noreturn)) mmu_handler(regs_t *ctx, uint8_t exception, void *self);

int mmu_map_area(uint64_t virt, uint32_t phys, uint32_t length, uint32_t prot);
int mmu_map_page(uint64_t virt, uint32_t phys, uint32_t prot);


#ifdef bug
#undef bug
#endif
#ifdef D
#undef D
#endif
#define D(x) x

#define __STR(x) #x
#define STR(x) __STR(x)

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

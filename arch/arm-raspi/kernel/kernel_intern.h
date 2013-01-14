/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef KERNEL_INTERN_H_
#define KERNEL_INTERN_H_

#include <aros/libcall.h>
#include <inttypes.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <stdio.h>
#include <stdarg.h>

#include <asm/bcm2835.h>

#undef KernelBase

#define KERNEL_PHYS_BASE        0x07800000
#define KERNEL_VIRT_BASE        0xff800000

#define gpioGPSET0 ((volatile unsigned int *)(GPSET0))
#define gpioGPCLR0 ((volatile unsigned int *)(GPCLR0))

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Exceptions[21];
    struct List         kb_Interrupts[64];
    struct MemHeader    *kb_SupervisorMem;
    struct MinList		kb_Modules;
//    context_t			*kb_FPUOwner;
    struct List			kb_DeadTasks;
    struct Task			*kb_LastDeadTask;
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
//    int                 (*in_Handler)(regs_t *, void *, void *);
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


void core_SetupMMU(void);
void core_SetupIntr(void);

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);

//void core_Cause(struct ExecBase *SysBase);

struct KernelBase *getKernelBase();

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
         struct KernelBase *, KernelBase, 12, Kernel);

static inline void bug(const char *format, ...)
{
    struct KernelBase *kbase = getKernelBase();
    va_list args;
    va_start(args, format);
    AROS_SLIB_ENTRY(KrnBug, Kernel, 12)(format, args, kbase);
    va_end(args);
}

uint32_t findNames(intptr_t addr, char **module, char **function);

#endif /*KERNEL_INTERN_H_*/

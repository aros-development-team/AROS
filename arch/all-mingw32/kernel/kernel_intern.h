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

//#include "syscall.h"

#define KERNEL_PHYS_BASE        0x00800000
#define KERNEL_VIRT_BASE        0xff800000

#define STACK_SIZE 4096

typedef void (*Exec_Callback)(struct ExecBase*);

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Exceptions[16];
    struct List         kb_Interrupts[64];
    struct MemHeader    *kb_SupervisorMem;
};

struct KernelBSS {
    void *addr;
    IPTR len;
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

struct KernelInterface {
    void (*StartScheduler)(Exec_Callback ExceptPtr, Exec_Callback DispatchPtr, struct ExecBase *ExecBasePtr);
};

extern struct HostInterface *HostIFace;
extern struct KernelInterface KernelIFace;

IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);

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
    va_list args;
    va_start(args, format);
    AROS_SLIB_ENTRY(KrnBug, Kernel)(format, args, NULL); /* Warning! It's a HACK (KernelBase == NULL)!!! */
    va_end(args);
}

#endif /*KERNEL_INTERN_H_*/

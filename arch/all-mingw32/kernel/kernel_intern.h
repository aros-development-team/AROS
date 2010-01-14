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
#include "hostinterface.h"

#define STACK_SIZE 4096

/* In Windows-hosted kernel exceptions and IRQs work in the following way:

   Exceptions are currently not used. They are reserved for implementing CPU
   exceptions handling (access violation, division by zero, etc).
   
   IRQs are used to receive events from emulated hardware. Hardware is mostly
   emulated using Windows threads running asynchronously to AROS. When the
   thread finishes its job it calls host-size KrnCauseIRQ() function in order
   to initiate an IRQ in AROS.
   
   Currently number of IRQs are fixed and they are used as following:
   IRQ 0 - main system periodic timer (50 Hz). Used internally by kernel.resource
           for task switching and VBlank emulation. Exec uses it as a VBLANK source.
           In current implementation it can not be caused manually using KernelCauseIRQ().
   IRQ 1 - Used by emul.handler to serve input from host's console.
   IRQ 2 - Used by graphics HIDD.
   IRQ 3 - Used by mouse HIDD.
   IRQ 4 - Used by keyboard HIDD.
   
   In future there can be much more drivers many of which would need an IRQ. In order to manage
   this i have an idea of implementing dynamic allocation of IRQs in some way.
   
   The whole described thing is experimental and subject to change.
*/

#define EXCEPTIONS_NUM 1

#define INT_TIMER 0

struct KernelBase {
    struct Node         kb_Node;
    void *              kb_MemPool;
    struct List         kb_Exceptions[EXCEPTIONS_NUM];
    struct List         kb_Interrupts;
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

#ifdef bug
#undef bug
#endif

#ifdef __AROS__
struct KernelInterface {
    long (*core_init)(unsigned long TimerPeriod, struct ExecBase **SysBasePointer, APTR *KernelBasePointer);
    long (*core_intr_disable)(void);
    long (*core_intr_enable)(void);
    void (*core_syscall)(unsigned long n);
    unsigned char (*core_is_super)(void);
    long (*core_exception)(void *ExceptionRecord, void *EstablisherFrame, void *ContextRecord, void *DispatcherContext);
};

extern struct HostInterface *HostIFace;
extern struct KernelInterface KernelIFace;
extern APTR KernelBase;

IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);

AROS_LD2(int, KrnBug,
         AROS_LDA(const char *, format, A0),
         AROS_LDA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel);

static inline void bug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    AROS_SLIB_ENTRY(KrnBug, Kernel)(format, args, KernelBase);
    va_end(args);
}

#else

#define SLEEP_MODE_OFF     0
#define SLEEP_MODE_PENDING 1
#define SLEEP_MODE_ON      2

extern DWORD SwitcherId;
extern DWORD *LastErrorPtr;
extern unsigned char Ints_Enabled;
extern unsigned char Sleep_Mode;
extern unsigned char PendingInts[256];
extern struct ExecBase **SysBasePtr;
extern struct KernelBase **KernelBasePtr;

void core_Dispatch(CONTEXT *regs, struct ExecBase *SysBase);
void core_Switch(CONTEXT *regs, struct ExecBase *SysBase);
void core_Schedule(CONTEXT *regs, struct ExecBase *SysBase);
void core_ExitInterrupt(CONTEXT *regs);
void core_Cause(struct ExecBase *SysBase);
long core_intr_enable(void);

static inline void core_LeaveInterrupt(struct ExecBase *SysBase)
{   
    if ((char )SysBase->IDNestCnt < 0)
        core_intr_enable();
}

#define bug printf

#endif

#endif /*KERNEL_INTERN_H_*/

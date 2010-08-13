#ifndef KERNEL_BASE_H_
#define KERNEL_BASE_H_

#include <aros/kernel.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <utility/tagitem.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>

#include <kernel_cpu.h>

#include "hostinterface.h"

#define STACK_SIZE 4096

/*
   In Windows-hosted kernel IRQs are used to receive events from emulated
   hardware. Hardware is mostly emulated using Windows threads running
   asynchronously to AROS. When the thread finishes its job it calls host-side
   KrnCauseIRQ() function in order to initiate an IRQ in AROS.

   IRQs are managed dynamically using host-side KrnAllocIRQ() and KrnFreeIRQ() functions
   except the following static allocations:

   IRQ 0 - main system periodic timer (50 Hz). Used internally by kernel.resource
           for task switching and VBlank emulation. Exec uses it as a VBLANK source.
           In current implementation it can not be caused manually using KernelCauseIRQ().

   The whole described thing is experimental and subject to change.
*/

#define IRQ_COUNT        1

#define INT_TIMER 0

struct KernelBase
{
    struct Node            kb_Node;
    void *                 kb_MemPool;
    struct List            kb_Exceptions[EXCEPTIONS_COUNT];
    /* IRQs are allocated dynamically so we never know how many of them we have.
       So put all handlers into single list. Use the fact that IRQ number is stored
       in in_nr */
    struct List            kb_Interrupts[1];
    struct MinList         kb_Modules;
    dbg_seg_t		  *kb_KernelModules;
    struct SignalSemaphore kb_ModSem;
};

#ifdef __AROS__
struct KernelInterface
{
    int (*core_init)(unsigned int TimerPeriod, struct ExecBase *SysBase, APTR *KernelBase);
    void (*core_intr_disable)(void);
    void (*core_intr_enable)(void);
    void (*core_syscall)(const unsigned int n);
    unsigned char (*core_is_super)(void);
    unsigned int (*core_protect)(void *addr, unsigned int len, unsigned int prot);
};

extern struct HostInterface *HostIFace;
extern struct KernelInterface KernelIFace;

#define krnSysCall KernelIFace.core_syscall

#else

#define SLEEP_MODE_OFF     0
#define SLEEP_MODE_PENDING 1
#define SLEEP_MODE_ON      2

extern DWORD SwitcherId;
extern DWORD *LastErrorPtr;
extern unsigned char Ints_Enabled;
extern unsigned char Sleep_Mode;
extern unsigned char PendingInts[256];
extern struct ExecBase *SysBase;
extern struct KernelBase *KernelBase;

void core_Dispatch(CONTEXT *regs, struct ExecBase *SysBase);
void core_Switch(CONTEXT *regs, struct ExecBase *SysBase);
void core_Schedule(CONTEXT *regs, struct ExecBase *SysBase);
void core_ExitInterrupt(CONTEXT *regs);
void core_Cause(struct ExecBase *SysBase);
void core_intr_enable(void);

#define bug printf

#endif

#endif /*KERNEL_BASE_H_*/

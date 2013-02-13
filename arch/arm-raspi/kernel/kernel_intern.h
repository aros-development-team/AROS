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

/*
// Enable support for paging memory to media..
#define RASPI_VIRTMEMSUPPORT
*/

/* Use system timer 3 for our scheduling heartbeat */
#define VBLANK_TIMER            3
#define VBLANK_INTERVAL         (1000000 / 50)

#define KERNEL_PHYS_BASE        0x07800000
#define KERNEL_VIRT_BASE        0xff800000

#define VFPEnable       0x40000000 
#define VFPSingle       0x300000 
#define VFPDouble       0xC00000 

#define gpioGPSET0 ((volatile unsigned int *)(GPSET0))
#define gpioGPCLR0 ((volatile unsigned int *)(GPCLR0))

void core_SetupMMU(void);
void core_SetupIntr(void);

void *KrnAddSysTimerHandler(struct KernelBase *);

intptr_t krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList);
struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList);
struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr);

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

#define VECTCOMMON_START \
    "           sub     sp, sp, #4*4           \n" \
    "           stmfd   sp!, {r0-r12}          \n" \
    "           mov     r0, sp                 \n" \
    "           mrs     r1, spsr               \n" \
    "           str     r1, [r0, #16*4]        \n" \
    "           str     lr, [r0, #15*4]        \n"

#define VECTCOMMON_END \
    "           add     r1, sp, #13*4          \n" \
    "           ldm     r1, {sp}^              \n" \
    "           add     r1, sp, #14*4          \n" \
    "           ldm     r1, {lr}^              \n" \
    "           ldr     lr, [sp, #15*4]        \n" \
    "           ldr     r1, [sp, #16*4]        \n" \
    "           msr     spsr, r1               \n" \
    "           ldmfd   sp!, {r0-r12}          \n" \
    "           add     sp, sp, #4*4           \n" \
    "           movs    pc, lr                 \n"

#define STORE_TASKSTATE(task, regs)                                             \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    int __task_reg_no;                                                          \
    for (__task_reg_no = 0; __task_reg_no < 12; __task_reg_no++)                \
    {                                                                           \
        ctx->r[__task_reg_no] = ((uint32_t *)regs)[__task_reg_no];              \
    }                                                                           \
    ctx->ip = ((uint32_t *)regs)[12];                                           \
    ctx->sp = task->tc_SPReg = ((uint32_t *)regs)[13];                          \
    ctx->lr = ((uint32_t *)regs)[14];                                           \
    ctx->pc = ((uint32_t *)regs)[15];                                           \
    ctx->cpsr = ((uint32_t *)regs)[16];

#define RESTORE_TASKSTATE(task, regs)                                           \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    int __task_reg_no;                                                          \
    for (__task_reg_no = 0; __task_reg_no < 12; __task_reg_no++)                \
    {                                                                           \
        ((uint32_t *)regs)[__task_reg_no] = ctx->r[__task_reg_no];              \
    }                                                                           \
    ((uint32_t *)regs)[12] = ctx->ip;                                           \
    ((uint32_t *)regs)[13] = ctx->sp = task->tc_SPReg;                          \
    ((uint32_t *)regs)[14] = ctx->lr;                                           \
    ((uint32_t *)regs)[15] = ctx->pc;                                           \
    ((uint32_t *)regs)[16] = ctx->cpsr;

#endif /*KERNEL_INTERN_H_*/

/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
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

#include "kernel_arm.h"

#undef KernelBase
struct KernelBase;

#define KERNEL_PHYS_BASE        0x07800000
#define KERNEL_VIRT_BASE        0xff800000

#define VFPEnable               (1 << 30)
#define VFPSingle               (3 << 20)
#define VFPDouble               (3 << 22)

void dt_set_root(void * r);
void *dt_find_node_by_phandle(uint32_t phandle);
void *dt_find_node(char *key);
void *dt_find_property(void *key, char *propname);
int dt_get_prop_len(void *prop);
void *dt_get_prop_value(void *prop);

void cpu_Probe(struct ARM_Implementation *);
void cpu_Init(struct ARM_Implementation *, struct TagItem *);

void platform_Init(struct ARM_Implementation *, struct TagItem *);

void core_SetupMMU(struct TagItem *msg);
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
#if DEBUG
#define D(x) x
#define DALLOCMEM(x)
#else
#define D(x)
#define DALLOCMEM(x)
#endif

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
    "           srsfd   sp!, #0x13              \n" \
    "           cpsid   i, #0x13                \n" \
    "           sub     sp, sp, #2*4            \n" \
    "           stmfd   sp!, {r0-r12}           \n" \
    "           sub     r0, sp, #4              \n" \
    "           strex   r1, r2, [r0]            \n" \
    "           mov     r0, sp                  \n" \
    "           ldr     ip, [r0, #16*4]         \n" \
    "           and     ip, ip, #0x1f           \n" \
    "           cmp     ip, #0x10               \n" \
    "           cmpne   ip, #0x1f               \n" \
    "           addeq   r1, r0, #13*4           \n" \
    "           stmeq   r1, {sp, lr}^           \n" \
    "           strne   lr, [r0, #14*4]         \n"

#define VECTCOMMON_END \
    "           sub     r0, sp, #4              \n" \
    "           strex   r1, r2, [r0]            \n" \
    "           ldr     ip, [sp, #16*4]         \n" \
    "           and     ip, ip, #0x1f           \n" \
    "           cmp     ip, #0x10               \n" \
    "           cmpne   ip, #0x1f               \n" \
    "           addeq   r1, sp, #13*4           \n" \
    "           ldmeq   r1, {sp, lr}^           \n" \
    "           ldrne   lr, [sp, #14*4]         \n" \
    "           ldmfd   sp!, {r0-r12}           \n" \
    "           add     sp, sp, #2*4            \n" \
    "           rfefd   sp!                     \n"

#define STORE_TASKSTATE(task, regs)                                             \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    int __task_reg_no;                                                          \
    for (__task_reg_no = 0; __task_reg_no < 12; __task_reg_no++)                \
    {                                                                           \
        ctx->r[__task_reg_no] = ((uint32_t *)regs)[__task_reg_no];              \
    }                                                                           \
    ctx->ip = ((uint32_t *)regs)[12];                                           \
    ctx->sp = ((uint32_t *)regs)[13];                                           \
    task->tc_SPReg = (void *)ctx->sp;                                           \
    ctx->lr = ((uint32_t *)regs)[14];                                           \
    ctx->pc = ((uint32_t *)regs)[15];                                           \
    ctx->cpsr = ((uint32_t *)regs)[16];                                         \
    if (__arm_arosintern.ARMI_Save_VFP_State) __arm_arosintern.ARMI_Save_VFP_State(ctx->fpuContext);

#define RESTORE_TASKSTATE(task, regs)                                           \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    int __task_reg_no;                                                          \
    for (__task_reg_no = 0; __task_reg_no < 12; __task_reg_no++)                \
    {                                                                           \
        ((uint32_t *)regs)[__task_reg_no] = ctx->r[__task_reg_no];              \
    }                                                                           \
    ((uint32_t *)regs)[12] = ctx->ip;                                           \
    ctx->sp = (intptr_t)task->tc_SPReg;                                         \
    ((uint32_t *)regs)[13] = ctx->sp;                                           \
    ((uint32_t *)regs)[14] = ctx->lr;                                           \
    ((uint32_t *)regs)[15] = ctx->pc;                                           \
    ((uint32_t *)regs)[16] = ctx->cpsr;                                         \
    if (__arm_arosintern.ARMI_Restore_VFP_State) __arm_arosintern.ARMI_Restore_VFP_State(ctx->fpuContext);

#endif /*KERNEL_INTERN_H_*/

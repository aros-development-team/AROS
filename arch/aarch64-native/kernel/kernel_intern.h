/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
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

#include <aros/aarch64/cpucontext.h>

#include "kernel_arm.h"

#undef KernelBase
struct KernelBase;

#define KERNEL_PHYS_BASE        0x07800000
#define KERNEL_VIRT_BASE        0xff800000

/* AArch64 MMU control bits */
#define ENABLE_MMU              (1 << 0)
#define ENABLE_D_CACHE          (1 << 2)
#define ENABLE_I_CACHE          (1 << 12)

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
    /* KrnBug is reached through KernelBase's jumptable; before KernelBase
     * exists (early kernel_cstart) the call would dereference NULL. Bail
     * out so early callers are no-ops -- they use uart_puts for tracing. */
    if (!kbase)
        return;
    va_start(args, format);
    AROS_SLIB_ENTRY(KrnBug, Kernel, 12)(format, args, kbase);
    va_end(args);
}

/*
 * FPU/NEON register block pointed to by ExceptionContext.fpuContext.
 * Layout is shared with the exception-frame FPU area and the
 * cpu_Save/Restore_FP_State() helpers: q0-q31 as 64 dwords, then
 * fpcr and fpsr.
 */
struct VFPContext
{
    UQUAD fpr[64];  /* q0-q31 (each 128 bits = 2 dwords) */
    UQUAD fpcr;
    UQUAD fpsr;
};

/*
 * AArch64 exception frame layout. The first part is struct
 * ExceptionContext (arch/aarch64-all/include/aros/cpucontext.h):
 *   [sp + 0]    x0-x28   (29 * 8 = 232 bytes)
 *   [sp + 232]  fp (x29)
 *   [sp + 240]  lr (x30)
 *   [sp + 248]  sp
 *   [sp + 256]  pc (elr_el1)
 *   [sp + 264]  cpsr (spsr_el1, ULONG)
 *   [sp + 268]  Flags (ULONG, ECF_FPU)
 *   [sp + 272]  fpuContext (points at sp + 288)
 * then, 16-byte aligned, the FPU block saved by vectors.S at entry
 * (C exception handlers and driver interrupt code freely use the
 * q-registers; only rom/kernel is compiled with -mgeneral-regs-only):
 *   [sp + 288]  q0-q31 (512 bytes)
 *   [sp + 800]  fpcr
 *   [sp + 808]  fpsr
 *   Total: 816 bytes, 16-byte aligned
 */
#define AARCH64_FRAME_SIZE  816
#define AARCH64_FRAME_FPU   288

/*
 * Store current task state from exception frame to ExceptionContext.
 * regs points to the exception frame on the kernel stack. The frame's
 * fpuContext points into the frame itself; the task context keeps its
 * own FPU block, so only the DATA is copied, never the pointer.
 */
#define STORE_TASKSTATE(task, regs)                                             \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    struct ExceptionContext *__frame = (struct ExceptionContext *)(regs);       \
    int __task_reg_no;                                                          \
    for (__task_reg_no = 0; __task_reg_no < 29; __task_reg_no++)                \
    {                                                                           \
        ctx->x[__task_reg_no] = __frame->x[__task_reg_no];                      \
    }                                                                           \
    ctx->fp = __frame->fp;                                                      \
    ctx->lr = __frame->lr;                                                      \
    ctx->sp = __frame->sp;                                                      \
    task->tc_SPReg = (void *)(IPTR)ctx->sp;                                     \
    ctx->pc = __frame->pc;                                                      \
    ctx->cpsr = __frame->cpsr;                                                  \
    ctx->Flags |= ECF_FPU;                                                      \
    for (__task_reg_no = 0; __task_reg_no < 66; __task_reg_no++)                \
    {                                                                           \
        ((UQUAD *)ctx->fpuContext)[__task_reg_no] =                             \
            ((UQUAD *)((char *)(regs) + AARCH64_FRAME_FPU))[__task_reg_no];     \
    }

/*
 * Restore task state from ExceptionContext to exception frame.
 */
#define RESTORE_TASKSTATE(task, regs)                                           \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    struct ExceptionContext *__frame = (struct ExceptionContext *)(regs);       \
    int __task_reg_no;                                                          \
    for (__task_reg_no = 0; __task_reg_no < 29; __task_reg_no++)                \
    {                                                                           \
        __frame->x[__task_reg_no] = ctx->x[__task_reg_no];                      \
    }                                                                           \
    __frame->fp = ctx->fp;                                                      \
    ctx->sp = (UQUAD)(IPTR)task->tc_SPReg;                                      \
    __frame->lr = ctx->lr;                                                      \
    __frame->sp = ctx->sp;                                                      \
    __frame->pc = ctx->pc;                                                      \
    __frame->cpsr = ctx->cpsr;                                                  \
    for (__task_reg_no = 0; __task_reg_no < 66; __task_reg_no++)                \
    {                                                                           \
        ((UQUAD *)((char *)(regs) + AARCH64_FRAME_FPU))[__task_reg_no] =        \
            ((UQUAD *)ctx->fpuContext)[__task_reg_no];                          \
    }

#endif /*KERNEL_INTERN_H_*/

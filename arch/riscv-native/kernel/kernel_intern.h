/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
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

#undef KernelBase
struct KernelBase;

#define __STR(x) #x
#define STR(x) __STR(x)

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

AROS_LD2(int, KrnBug,
         AROS_LDA(const char *, format, A0),
         AROS_LDA(va_list, args, A1),
         struct KernelBase *, KernelBase, 12, Kernel);

#ifdef bug
#undef bug
#endif
static inline void _kibug(struct KernelBase *kbase, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    AROS_SLIB_ENTRY(KrnBug, Kernel, 12)(format, args, kbase);
    va_end(args);
}
#define bug(fmt,args...) _kibug(KernelBase, fmt, ##args)

#define STORE_TASKSTATE(task, regs)                 \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    ctx->x[0] = ((uint32_t *)regs)[0];              \
    ctx->x[1] = ((uint32_t *)regs)[1];              \
    ctx->x[2] = ((uint32_t *)regs)[2];              \
    ctx->x[3] = ((uint32_t *)regs)[3];              \
    ctx->x[4] = ((uint32_t *)regs)[4];              \
    ctx->x[5] = ((uint32_t *)regs)[5];              \
    ctx->x[6] = ((uint32_t *)regs)[6];              \
    ctx->x[7] = ((uint32_t *)regs)[7];              \
    ctx->x[8] = ((uint32_t *)regs)[8];              \
    ctx->x[9] = ((uint32_t *)regs)[9];              \
    ctx->x[10] = ((uint32_t *)regs)[10];            \
    ctx->x[11] = ((uint32_t *)regs)[11];            \
    ctx->x[12] = ((uint32_t *)regs)[12];            \
    ctx->x[13] = ((uint32_t *)regs)[13];            \
    ctx->x[14] = ((uint32_t *)regs)[14];            \
    ctx->x[15] = ((uint32_t *)regs)[15];            \
    ctx->x[16] = ((uint32_t *)regs)[16];            \
    ctx->x[17] = ((uint32_t *)regs)[17];            \
    ctx->x[18] = ((uint32_t *)regs)[18];            \
    ctx->x[19] = ((uint32_t *)regs)[19];            \
    ctx->x[20] = ((uint32_t *)regs)[20];            \
    ctx->x[21] = ((uint32_t *)regs)[21];            \
    ctx->x[22] = ((uint32_t *)regs)[22];            \
    ctx->x[23] = ((uint32_t *)regs)[23];            \
    ctx->x[24] = ((uint32_t *)regs)[24];            \
    ctx->x[25] = ((uint32_t *)regs)[25];            \
    ctx->x[26] = ((uint32_t *)regs)[26];            \
    ctx->x[27] = ((uint32_t *)regs)[27];            \
    ctx->x[28] = ((uint32_t *)regs)[28];            \
    ctx->pc = ((uint32_t *)regs)[29];               \
    task->tc_SPReg = (void *)ctx->sp;

#define RESTORE_TASKSTATE(task, regs)               \
    struct ExceptionContext *ctx = task->tc_UnionETask.tc_ETask->et_RegFrame;   \
    ((uint32_t *)regs)[0] = ctx->x[0];              \
    ctx->sp = (intptr_t)task->tc_SPReg;             \
    ((uint32_t *)regs)[2] = ctx->x[2];              \
    ((uint32_t *)regs)[3] = ctx->x[3];              \
    ((uint32_t *)regs)[4] = ctx->x[4];              \
    ((uint32_t *)regs)[5] = ctx->x[5];              \
    ((uint32_t *)regs)[6] = ctx->x[6];              \
    ((uint32_t *)regs)[7] = ctx->x[7];              \
    ((uint32_t *)regs)[8] = ctx->x[8];              \
    ((uint32_t *)regs)[9] = ctx->x[9];              \
    ((uint32_t *)regs)[10] = ctx->x[10];            \
    ((uint32_t *)regs)[11] = ctx->x[11];            \
    ((uint32_t *)regs)[12] = ctx->x[12];            \
    ((uint32_t *)regs)[13] = ctx->x[13];            \
    ((uint32_t *)regs)[14] = ctx->x[14];            \
    ((uint32_t *)regs)[15] = ctx->x[15];            \
    ((uint32_t *)regs)[16] = ctx->x[16];            \
    ((uint32_t *)regs)[17] = ctx->x[17];            \
    ((uint32_t *)regs)[18] = ctx->x[18];            \
    ((uint32_t *)regs)[19] = ctx->x[19];            \
    ((uint32_t *)regs)[20] = ctx->x[20];            \
    ((uint32_t *)regs)[21] = ctx->x[21];            \
    ((uint32_t *)regs)[22] = ctx->x[22];            \
    ((uint32_t *)regs)[23] = ctx->x[23];            \
    ((uint32_t *)regs)[24] = ctx->x[24];            \
    ((uint32_t *)regs)[25] = ctx->x[25];            \
    ((uint32_t *)regs)[26] = ctx->x[26];            \
    ((uint32_t *)regs)[27] = ctx->x[27];            \
    ((uint32_t *)regs)[28] = ctx->x[28];            \
    ((uint32_t *)regs)[29] = ctx->pc;

static inline int GetCPUNumber() {
    int _mhartid;
    __asm__ volatile("csrr %0, mhartid" : "=r"(_mhartid));
    return _mhartid;
}

#endif /*KERNEL_INTERN_H_*/

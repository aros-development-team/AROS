#ifndef AROS_RISCV_CPUCONTEXT_H
#define AROS_RISCV_CPUCONTEXT_H

/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context definition for RISC-V processors
    Lang: english
*/

/* 32 (x) registers, minus x0(zero), x3(gp) and x4(tp)                  */
#define RISCV_REGSAVE_CNT   (32 - 3)
/* How many args are passed in registers (a0-a7)                        */
#define RISCV_FUNCREG_CNT   8

struct ExceptionContext
{
    union {
        ULONG x[RISCV_REGSAVE_CNT];     /* General purpose registers    */
        struct {
            ULONG ra;                   /* 0 = x1                       */
            ULONG sp;                   /* x2                           */
            ULONG t0;                   /* x5                           */
            ULONG t1;
            ULONG t2;
#define REG_X_FP_OFF    5
            ULONG fp;
            ULONG s1;
#define REG_X_Ax_OFF    7
            ULONG a0;
            ULONG a1;
            ULONG a2;
            ULONG a3;
            ULONG a4;
            ULONG a5;
            ULONG a6;
            ULONG a7;
            ULONG s2;
            ULONG s3;
            ULONG s4;
            ULONG s5;
            ULONG s6;
            ULONG s7;
            ULONG s8;
            ULONG s9;
            ULONG s10;
            ULONG s11;
            ULONG t3;
            ULONG t4;
            ULONG t5;
            ULONG t6;
        };
    };
    ULONG pc;		                /* sepc/mepc when trapped	    */
    ULONG sr;		                /* sstatus/mstatus when trapped   */
    ULONG Flags;	                /* ECF_* flags			    */
    APTR  fpuContext;               /* FPU register state, if saved   */
    APTR  vecContext;               /* Vector register state, if saved */
};

/* FPU register state (FLEN=64, D extension) */
struct FpuContext
{
    UQUAD f[32];                    /* f0-f31                         */
    ULONG fcsr;
};

/*
 * Vector register state (V extension, detected at runtime). VLEN - and
 * therefore the size of the register file - is only known at runtime
 * (from the vlenb CSR), so the v0-v31 data follows the header as
 * 32 * vlenb bytes.
 */
struct VectorContext
{
    ULONG vstart;
    ULONG vtype;
    ULONG vl;
    ULONG vcsr;
    ULONG vlenb;                    /* bytes per v register           */
    UBYTE v[];                      /* 32 * vlenb bytes, 16-aligned   */
};

/* ExceptionContext Flags */
#define ECF_FPU     0x0001 /* fpuContext is valid */
#define ECF_VECTOR  0x0002 /* vecContext is valid */

/* CPU privilege modes (RISC-V privileged spec) */
#define CPUMODE_USER            0x00
#define CPUMODE_SUPERVISOR      0x01
#define CPUMODE_MACHINE         0x03

#endif

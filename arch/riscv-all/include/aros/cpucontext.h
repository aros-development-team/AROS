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
        IPTR  x[RISCV_REGSAVE_CNT];     /* General purpose registers    */
        struct {
            IPTR  ra;                   /* 0 = x1                       */
            IPTR  sp;                   /* x2                           */
            IPTR  t0;                   /* x5                           */
            IPTR  t1;
            IPTR  t2;
#define REG_X_FP_OFF    5
            IPTR  fp;
            IPTR  s1;
#define REG_X_Ax_OFF    7
            IPTR  a0;
            IPTR  a1;
            IPTR  a2;
            IPTR  a3;
            IPTR  a4;
            IPTR  a5;
            IPTR  a6;
            IPTR  a7;
            IPTR  s2;
            IPTR  s3;
            IPTR  s4;
            IPTR  s5;
            IPTR  s6;
            IPTR  s7;
            IPTR  s8;
            IPTR  s9;
            IPTR  s10;
            IPTR  s11;
            IPTR  t3;
            IPTR  t4;
            IPTR  t5;
            IPTR  t6;
        };
    };
    IPTR  pc;		                /* sepc/mepc when trapped	    */
    IPTR  sr;		                /* sstatus/mstatus when trapped   */
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
    IPTR  vstart;
    IPTR  vtype;
    IPTR  vl;
    IPTR  vcsr;
    IPTR  vlenb;                    /* bytes per v register           */
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

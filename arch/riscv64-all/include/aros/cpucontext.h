#ifndef AROS_RISCV64_CPUCONTEXT_H
#define AROS_RISCV64_CPUCONTEXT_H

/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context definition for 64bit RISC-V processors
    Lang: english
*/

/* 32 (x) registers, minus x0(zero), x3(gp) and x4(tp)                  */
#define RISCV_REGSAVE_CNT   (32 - 3)
/* How many args are passed in registers (a0-a7)                        */
#define RISCV_FUNCREG_CNT   8

struct ExceptionContext
{
    union {
        UQUAD x[RISCV_REGSAVE_CNT];     /* General purpose registers    */
        struct {
            UQUAD ra;                   /* 0 = x1                       */
            UQUAD sp;                   /* x2                           */
            UQUAD t0;                   /* x5                           */
            UQUAD t1;
            UQUAD t2;
#define REG_X_FP_OFF    5
            UQUAD fp;
            UQUAD s1;
#define REG_X_Ax_OFF    7
            UQUAD a0;
            UQUAD a1;
            UQUAD a2;
            UQUAD a3;
            UQUAD a4;
            UQUAD a5;
            UQUAD a6;
            UQUAD a7;
            UQUAD s2;
            UQUAD s3;
            UQUAD s4;
            UQUAD s5;
            UQUAD s6;
            UQUAD s7;
            UQUAD s8;
            UQUAD s9;
            UQUAD s10;
            UQUAD s11;
            UQUAD t3;
            UQUAD t4;
            UQUAD t5;
            UQUAD t6;
        };
    };
    UQUAD pc;		                /* sepc/mepc when trapped	    */
    UQUAD sr;		                /* sstatus/mstatus when trapped   */
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
    UQUAD vstart;
    UQUAD vtype;
    UQUAD vl;
    UQUAD vcsr;
    UQUAD vlenb;                    /* bytes per v register           */
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

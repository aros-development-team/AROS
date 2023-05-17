#ifndef AROS_RISCV_CPUCONTEXT_H
#define AROS_RISCV_CPUCONTEXT_H

/*
    Copyright © 2023, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CPU context definition for RISC-V processors
    Lang: english
*/

/* 32 (x) registers, minus x0(zero), x3(gp) and x4(tp)                  */
#define RISCV_REGSAVE_CNT   (32 - 3)
/* How many args to pass in registers                                   */
#define RISCV_FUNCREG_CNT   4

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
    ULONG pc;		                /* csrrr/mepc				    */
    UWORD Flags;	                /* Currently reserved		    */
};

/* CPU modes */
#define CPUMODE_MACHINE         0x13
#define CPUMODE_SUPERVISOR      0x13
#define CPUMODE_USER            0x10

#endif

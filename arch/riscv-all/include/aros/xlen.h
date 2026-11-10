#ifndef AROS_RISCV_XLEN_H
#define AROS_RISCV_XLEN_H

/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: register width selection for RISC-V (RV32 and RV64).
    Lang: english

    C code (genmodule.h's inline asm stubs) gets the load, store and
    register size as strings. Assembler files also get the generated
    asm.h for their CPU, and SZREG, REG_S and REG_L for their loads and
    stores; those are defined only under __ASSEMBLER__, as C sources may
    use the same names for themselves.
*/

#if (__riscv_xlen == 64)
#define __AROS_RV_LX_STR    "ld"
#define __AROS_RV_SX_STR    "sd"
#define __AROS_RV_SZREG_STR "8"
#ifdef __ASSEMBLER__
#include "aros/riscv64/asm.h"
#define SZREG   8
#define REG_S   sd
#define REG_L   ld
#endif
#elif (__riscv_xlen == 32)
#define __AROS_RV_LX_STR    "lw"
#define __AROS_RV_SX_STR    "sw"
#define __AROS_RV_SZREG_STR "4"
#ifdef __ASSEMBLER__
#include "aros/riscv/asm.h"
#define SZREG   4
#define REG_S   sw
#define REG_L   lw
#endif
#else
#error "aros/riscv/xlen.h: __riscv_xlen must be 32 or 64"
#endif

#endif /* AROS_RISCV_XLEN_H */

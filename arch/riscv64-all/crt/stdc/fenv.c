/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: FPU environment for 64bit risc-v processors.

    The fcsr layout is identical on RV32 and RV64, so reuse the riscv
    implementation.
*/

#include "../../../riscv-all/crt/stdc/fenv.c"

/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: atomic operations for risc-v processors
    Lang: english
*/

#ifndef AROS_RISCV_ATOMIC_H
#define AROS_RISCV_ATOMIC_H

/*
    Word (and, on RV64, doubleword) operations map directly onto the "A"
    extension AMO instructions; the compiler emits masked LR/SC sequences
    for the sub-word widths.
*/

#define __AROS_ATOMIC_INC_B(var) __atomic_fetch_add(&(var), 1, __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_INC_W(var) __atomic_fetch_add(&(var), 1, __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_INC_L(var) __atomic_fetch_add(&(var), 1, __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_DEC_B(var) __atomic_fetch_sub(&(var), 1, __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_DEC_W(var) __atomic_fetch_sub(&(var), 1, __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_DEC_L(var) __atomic_fetch_sub(&(var), 1, __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_AND_B(var, mask) __atomic_fetch_and(&(var), (mask), __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_AND_W(var, mask) __atomic_fetch_and(&(var), (mask), __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_AND_L(var, mask) __atomic_fetch_and(&(var), (mask), __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_OR_B(var, mask) __atomic_fetch_or(&(var), (mask), __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_OR_W(var, mask) __atomic_fetch_or(&(var), (mask), __ATOMIC_SEQ_CST)
#define __AROS_ATOMIC_OR_L(var, mask) __atomic_fetch_or(&(var), (mask), __ATOMIC_SEQ_CST)

#endif /* AROS_RISCV_ATOMIC_H */

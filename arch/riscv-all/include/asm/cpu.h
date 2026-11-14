#ifndef ASM_RISCV_CPU_H
#define ASM_RISCV_CPU_H

/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: assembler-level specific definitions for riscv CPUs,
          usable from both RV32 and RV64 code.
    Lang: english
*/

#include <asm/riscv/csr.h>  /* CSR fields, the kernel mode's names, rv_csr_name(),
                                rv_intr_disable/restore */

#ifdef __cplusplus
extern "C" {
#endif

/* Memory barriers and hints */
static inline void fence(void)    { asm volatile("fence"      : : : "memory"); }
static inline void fence_r(void)  { asm volatile("fence r,rw" : : : "memory"); }
static inline void fence_w(void)  { asm volatile("fence w,rw" : : : "memory"); }
/* Zifencei is optional in the RVA22 profile - name it explicitly */
static inline void fence_i(void)
{
    asm volatile(".option push\n"
                 ".option arch, +zifencei\n"
                 "fence.i\n"
                 ".option pop" : : : "memory");
}
static inline void wfi(void)      { asm volatile("wfi"); }

/*
 * CSR accessors. 'csr' must be a compile time CSR name or number, or a
 * macro that expands to one (RV_CSR_STATUS, CSR_VLENB).
 */
#define csr_read(csr)                                           \
({                                                              \
    unsigned long __v;                                          \
    asm volatile("csrr %0, " rv_csr_name(csr)                   \
                 : "=r"(__v) : : "memory");                     \
    __v;                                                        \
})

#define csr_write(csr, val)                                     \
do {                                                            \
    unsigned long __v = (unsigned long)(val);                   \
    asm volatile("csrw " rv_csr_name(csr) ", %0"                \
                 : : "rK"(__v) : "memory");                     \
} while (0)

#define csr_swap(csr, val)                                      \
({                                                              \
    unsigned long __v = (unsigned long)(val);                   \
    asm volatile("csrrw %0, " rv_csr_name(csr) ", %1"           \
                 : "=r"(__v) : "rK"(__v) : "memory");           \
    __v;                                                        \
})

#define csr_set(csr, mask)                                      \
do {                                                            \
    unsigned long __v = (unsigned long)(mask);                  \
    asm volatile("csrs " rv_csr_name(csr) ", %0"                \
                 : : "rK"(__v) : "memory");                     \
} while (0)

#define csr_clear(csr, mask)                                    \
do {                                                            \
    unsigned long __v = (unsigned long)(mask);                  \
    asm volatile("csrc " rv_csr_name(csr) ", %0"                \
                 : : "rK"(__v) : "memory");                     \
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* ASM_RISCV_CPU_H */

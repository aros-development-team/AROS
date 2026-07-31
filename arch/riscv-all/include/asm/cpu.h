#ifndef ASM_RISCV_CPU_H
#define ASM_RISCV_CPU_H

/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: assembler-level specific definitions for riscv CPUs,
          usable from both RV32 and RV64 code.
    Lang: english
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Memory barriers and hints */
static inline void fence(void)    { asm volatile("fence"      : : : "memory"); }
static inline void fence_r(void)  { asm volatile("fence r,rw" : : : "memory"); }
static inline void fence_w(void)  { asm volatile("fence w,rw" : : : "memory"); }
static inline void fence_i(void)  { asm volatile("fence.i"    : : : "memory"); }
static inline void wfi(void)      { asm volatile("wfi"); }

/* CSR accessors. 'csr' must be a compile time CSR name or number. */
#define csr_read(csr)                                           \
({                                                              \
    unsigned long __v;                                          \
    asm volatile("csrr %0, " #csr : "=r"(__v) : : "memory");    \
    __v;                                                        \
})

#define csr_write(csr, val)                                     \
do {                                                            \
    unsigned long __v = (unsigned long)(val);                   \
    asm volatile("csrw " #csr ", %0" : : "rK"(__v) : "memory"); \
} while (0)

#define csr_swap(csr, val)                                      \
({                                                              \
    unsigned long __v = (unsigned long)(val);                   \
    asm volatile("csrrw %0, " #csr ", %1"                       \
                 : "=r"(__v) : "rK"(__v) : "memory");           \
    __v;                                                        \
})

#define csr_set(csr, mask)                                      \
do {                                                            \
    unsigned long __v = (unsigned long)(mask);                  \
    asm volatile("csrs " #csr ", %0" : : "rK"(__v) : "memory"); \
} while (0)

#define csr_clear(csr, mask)                                    \
do {                                                            \
    unsigned long __v = (unsigned long)(mask);                  \
    asm volatile("csrc " #csr ", %0" : : "rK"(__v) : "memory"); \
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* ASM_RISCV_CPU_H */

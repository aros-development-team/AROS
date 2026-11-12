#ifndef ASM_RISCV_CPU_H
#define ASM_RISCV_CPU_H

/*
    Copyright (c) 2023-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: assembler-level specific definitions for riscv CPUs,
          usable from both RV32 and RV64 code.
    Lang: english
*/

#include <aros/config.h>    /* __AROSPLATFORM_RISCV_MMODE__ */

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
 * sstatus fields. The FS and VS dirty-tracking fields drive lazy FPU and
 * vector context switching: the scheduler only saves the unit's state
 * when the field reads back as DIRTY, and traps on first use when it is
 * set to OFF. VS reads as a hardwired zero when the vector extension is
 * not implemented, which is also how its presence is detected at runtime.
 */
#define SSTATUS_SIE         0x00000002UL
#define SSTATUS_SPIE        0x00000020UL
#define SSTATUS_UBE         0x00000040UL
#define SSTATUS_SPP         0x00000100UL
#define SSTATUS_VS          0x00000600UL /* Vector unit state (V ext)   */
#define SSTATUS_VS_OFF      0x00000000UL
#define SSTATUS_VS_INITIAL  0x00000200UL
#define SSTATUS_VS_CLEAN    0x00000400UL
#define SSTATUS_VS_DIRTY    0x00000600UL
#define SSTATUS_FS          0x00006000UL /* FPU state                   */
#define SSTATUS_FS_OFF      0x00000000UL
#define SSTATUS_FS_INITIAL  0x00002000UL
#define SSTATUS_FS_CLEAN    0x00004000UL
#define SSTATUS_FS_DIRTY    0x00006000UL
#define SSTATUS_SUM         0x00040000UL
#define SSTATUS_MXR         0x00080000UL

/* sie interrupt-enable bits */
#define SIE_SSIE            0x00000002UL /* Supervisor software (IPI) */
#define SIE_STIE            0x00000020UL /* Supervisor timer          */
#define SIE_SEIE            0x00000200UL /* Supervisor external       */

/* sip interrupt-pending bits */
#define SIP_SSIP            0x00000002UL /* Supervisor software (IPI) */

/* scause interrupt codes (with the top bit set) */
#define SCAUSE_IRQ_SSI      1
#define SCAUSE_IRQ_STI      5
#define SCAUSE_IRQ_SEI      9

/* mstatus fields; FS and VS sit at the same bits as in sstatus */
#define MSTATUS_MIE         0x00000008UL
#define MSTATUS_MPIE        0x00000080UL
#define MSTATUS_MPP         0x00001800UL /* 3 = M-mode                  */

/* mie interrupt-enable bits */
#define MIE_MSIE            0x00000008UL /* Machine software (IPI)    */

/*
 * The CSRs and bits of the privilege mode the kernel runs in, chosen at
 * build time: supervisor mode (under SBI firmware) unless the platform
 * defines __AROSPLATFORM_RISCV_MMODE__ in aros/config.h. Shared code
 * uses these names; the S and M sets above stay for code that is
 * specific to one mode.
 */
#if defined(__AROSPLATFORM_RISCV_MMODE__)
#define RV_CSR_STATUS           mstatus
#define RV_CSR_IE               mie
#define RV_STATUS_IE            MSTATUS_MIE
#define RV_STATUS_PIE           MSTATUS_MPIE
#define RV_STATUS_PP_KERNEL     MSTATUS_MPP
#define RV_IE_SOFT              MIE_MSIE
#else
#define RV_CSR_STATUS           sstatus
#define RV_CSR_IE               sie
#define RV_STATUS_IE            SSTATUS_SIE
#define RV_STATUS_PIE           SSTATUS_SPIE
#define RV_STATUS_PP_KERNEL     SSTATUS_SPP
#define RV_IE_SOFT              SIE_SSIE
#endif

/* FPU and vector state fields, the same in both modes */
#define RV_STATUS_VS            SSTATUS_VS
#define RV_STATUS_VS_INITIAL    SSTATUS_VS_INITIAL
#define RV_STATUS_FS_INITIAL    SSTATUS_FS_INITIAL

/*
 * Vector extension CSR numbers. Numeric so they assemble without V in
 * the build's -march; accessing them traps unless the status VS field is
 * enabled.
 */
#define CSR_VSTART          0x008
#define CSR_VCSR            0x00F
#define CSR_VL              0xC20
#define CSR_VTYPE           0xC21
#define CSR_VLENB           0xC22

/*
 * CSR accessors. 'csr' must be a compile time CSR name or number, or a
 * macro that expands to one (RV_CSR_STATUS, CSR_VLENB). rv_csr_name()
 * gives it as a string for asm text of its own.
 */
#define __rv_csr_str(csr)   #csr
#define rv_csr_name(csr)    __rv_csr_str(csr)

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

/* Clear the bits in 'mask' and return the old value, in one instruction */
#define csr_read_clear(csr, mask)                               \
({                                                              \
    unsigned long __v = (unsigned long)(mask);                  \
    asm volatile("csrrc %0, " rv_csr_name(csr) ", %1"           \
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

/*
 * Mask interrupts on this hart and return whether they were enabled;
 * rv_intr_restore() enables them again only if they were, so the pair
 * nests. The IPI is an ordinary software interrupt, so this also holds
 * it off: there is no separate FIQ-like level as on ARM.
 */
static inline unsigned int rv_intr_disable(void)
{
    return (unsigned int)(csr_read_clear(RV_CSR_STATUS, RV_STATUS_IE)
                          & RV_STATUS_IE);
}

static inline void rv_intr_restore(unsigned int prev)
{
    if (prev)
        csr_set(RV_CSR_STATUS, RV_STATUS_IE);
}

#ifdef __cplusplus
}
#endif

#endif /* ASM_RISCV_CPU_H */

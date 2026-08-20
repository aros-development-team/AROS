/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AArch64 modular startup for Raspberry Pi bootstrap
*/

    .section .aros.startup, "ax"
    .globl _start
    .globl bootstrap
    .type _start, %function
    .type bootstrap, %function

    /*
     * EL2 -> EL1 transition macro per ARM Architecture Reference Manual (DDI 0487).
     *
     * Configures EL2 to allow EL1 access to timers and FP/SIMD,
     * sets EL1 execution state to AArch64, disables coprocessor traps,
     * and returns to EL1 via eret.
     */
    .macro el2_to_el1, tmp1, tmp2

    /* D13.2.27: enable EL1 physical and virtual counter access */
    mrs     \tmp1, cnthctl_el2
    orr     \tmp1, \tmp1, #0x3
    msr     cnthctl_el2, \tmp1
    msr     cntvoff_el2, xzr

    /* D13.2.141/142: copy MIDR/MPIDR to virtualization registers */
    mrs     \tmp1, midr_el1
    mrs     \tmp2, mpidr_el1
    msr     vpidr_el2, \tmp1
    msr     vmpidr_el2, \tmp2

    /* D13.2.30: disable all coprocessor traps to EL2 */
    mov     \tmp1, #0x33FF
    msr     cptr_el2, \tmp1
    msr     hstr_el2, xzr

    /* D13.2.36: enable FP/SIMD at EL1 (FPEN bits [21:20] = 0b11) */
    mov     \tmp1, #(3 << 20)
    msr     cpacr_el1, \tmp1

    /* D13.2.47: EL1 executes in AArch64 (RW bit [31] = 1) */
    mov     \tmp1, #(1 << 31)
    msr     hcr_el2, \tmp1

    /* D13.2.113: SCTLR_EL1 RES1 bits, MMU/caches off */
    movz    \tmp1, #0x0800
    movk    \tmp1, #0x30D0, lsl #16
    msr     sctlr_el1, \tmp1

    /* D13.2.109: return to EL1h (M[3:0]=0b0100) with DAIF masked */
    mov     \tmp1, #0x3C5
    msr     spsr_el2, \tmp1
    adr     \tmp1, 1f
    msr     elr_el2, \tmp1
    eret
1:
    .endm

_start:
bootstrap:
    /* Only let core 0 boot; secondary cores spin */
    mrs     x4, mpidr_el1
    and     x4, x4, #0xFF       /* Aff0 = core ID */
    cbnz    x4, secondary_spin

    /* Save DTB pointer (firmware passes it in x0) */
    mov     x20, x0

    /* Check current exception level (bits [3:2]) */
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    cmp     x0, #3              /* EL3 */
    b.eq    from_el3
    cmp     x0, #2              /* EL2 */
    b.eq    from_el2
    b       at_el1              /* EL1 */

from_el3:
    mrs     x0, scr_el3
    orr     x0, x0, #(1 << 10)  /* RW bit - EL2 is AArch64 */
    orr     x0, x0, #(1 << 0)   /* NS bit - Non-secure */
    msr     scr_el3, x0
    mov     x0, #0x3c9          /* SPSR_EL3: EL2h, DAIF masked */
    msr     spsr_el3, x0
    adr     x0, from_el2
    msr     elr_el3, x0
    eret

from_el2:
    el2_to_el1 x0, x1

at_el1:
    /* Ensure safe SCTLR_EL1 when entered directly at EL1 */
    movz    x0, #0x0800
    movk    x0, #0x30D0, lsl #16
    msr     sctlr_el1, x0
    isb

    /* Set up dual stacks:
     * SP_EL1 = 0x00380000 (exception/IRQ stack)
     * SP_EL0 = 0x00800000 (main kernel bootstrap stack)
     */
    msr     spsel, #1           /* select SP_EL1 */
    ldr     x0, =0x00380000
    mov     sp, x0              /* SP_EL1 = exception stack */
    msr     spsel, #0           /* select SP_EL0 */
    ldr     x0, =0x00800000
    mov     sp, x0              /* SP_EL0 = main stack */

    /* Install exception vector table */
    adr     x0, boot_vectors
    msr     vbar_el1, x0
    isb

    /* Enable FP/NEON (CPACR_EL1: FPEN = 0b11) */
    mov     x0, #(3 << 20)
    msr     cpacr_el1, x0
    isb

    /* Restore DTB pointer as first argument to boot() */
    mov     x0, x20

    /* Branch to C boot entry */
    bl      boot

    /* Should not return - hang if it does */
hang:
    wfe
    b       hang

secondary_spin:
    wfe
    b       secondary_spin

/*
 * Minimal exception vector table for bootstrap diagnostics.
 * Layout per ARM ARM D1.10.2: 4 groups x 4 vectors x 128 bytes.
 */
    .balign 2048
boot_vectors:
    /* Group 0: Current EL with SP_EL0 */
    b       boot_sync_handler   /* Synchronous */
    .balign 128
    b       hang                /* IRQ */
    .balign 128
    b       hang                /* FIQ */
    .balign 128
    b       hang                /* SError */
    .balign 128

    /* Group 1: Current EL with SP_ELx */
    b       boot_sync_handler   /* Synchronous */
    .balign 128
    b       hang                /* IRQ */
    .balign 128
    b       hang                /* FIQ */
    .balign 128
    b       hang                /* SError */
    .balign 128

    /* Group 2: Lower EL using AArch64 */
    b       hang
    .balign 128
    b       hang
    .balign 128
    b       hang
    .balign 128
    b       hang
    .balign 128

    /* Group 3: Lower EL using AArch32 */
    b       hang
    .balign 128
    b       hang
    .balign 128
    b       hang
    .balign 128
    b       hang
    .balign 128

boot_sync_handler:
    /* Decode and print exception registers via C helper */
    mrs     x0, esr_el1
    mrs     x1, elr_el1
    mrs     x2, far_el1
    bl      boot_exception_handler
    b       hang

    .section .text
    .byte 0
    /* Assemblers do not concatenate adjacent string literals (C does);
     * emit the pieces with .ascii and terminate with .asciz. */
    .ascii "$VER: arosraspi-aarch64.img v40.46 ("
    .ascii __DATE__
    .asciz ")"
    .byte 0

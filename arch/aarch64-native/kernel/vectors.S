/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    AArch64 Exception Vector Table and entry/exit code.

    Exception frame layout. The first 280 bytes are struct
    ExceptionContext (arch/aarch64-all/include/aros/cpucontext.h):
      [sp + 0]    x0-x28   (29 * 8 = 232 bytes)
      [sp + 232]  fp (x29)
      [sp + 240]  lr (x30)
      [sp + 248]  sp
      [sp + 256]  pc (elr_el1)
      [sp + 264]  cpsr (spsr_el1, 32-bit)
      [sp + 268]  Flags (32-bit, ECF_FPU)
      [sp + 272]  fpuContext (points at the FPU block below)
    then, 16-byte aligned, the FPU block:
      [sp + 288]  q0-q31   (32 * 16 = 512 bytes)
      [sp + 800]  fpcr
      [sp + 808]  fpsr
      Total: 816 bytes, 16-byte aligned

    The FP/SIMD state is saved/restored at exception entry/exit: C
    exception handlers and driver interrupt code freely use q-registers
    (only rom/kernel is compiled with -mgeneral-regs-only), so deferring
    the save to the context switch would hand handler-clobbered values
    to the interrupted task.
*/

#define FRAME_SIZE  816
#define FRAME_X0    0
#define FRAME_FP    232
#define FRAME_LR    240
#define FRAME_SP    248
#define FRAME_PC    256
#define FRAME_CPSR  264
#define FRAME_FLAGS 268
#define FRAME_FPCTX 272
#define FRAME_FPU   288
#define FRAME_FPCR  800
#define FRAME_FPSR  808

/* Must match ECF_FPU in cpucontext.h */
#define ECF_FPU_VAL 0x0001

/* ================================================================== */
/*  Save/Restore macros                                               */
/* ================================================================== */

/*
 * Save all registers to the exception frame on the kernel stack.
 * \from_el0: 1 if exception came from EL0, 0 if from EL1
 */
.macro save_context, from_el0
    sub     sp, sp, #FRAME_SIZE
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    stp     x8, x9, [sp, #64]
    stp     x10, x11, [sp, #80]
    stp     x12, x13, [sp, #96]
    stp     x14, x15, [sp, #112]
    stp     x16, x17, [sp, #128]
    stp     x18, x19, [sp, #144]
    stp     x20, x21, [sp, #160]
    stp     x22, x23, [sp, #176]
    stp     x24, x25, [sp, #192]
    stp     x26, x27, [sp, #208]
    stp     x28, x29, [sp, #224]
    str     x30, [sp, #FRAME_LR]

    .if \from_el0
    mrs     x21, sp_el0
    .else
    add     x21, sp, #FRAME_SIZE
    .endif
    str     x21, [sp, #FRAME_SP]

    mrs     x21, elr_el1
    str     x21, [sp, #FRAME_PC]

    /* cpsr and Flags are 32-bit fields side by side */
    mrs     x21, spsr_el1
    str     w21, [sp, #FRAME_CPSR]
    mov     w21, #ECF_FPU_VAL
    str     w21, [sp, #FRAME_FLAGS]

    /* fpuContext points at the FPU block within this frame */
    add     x21, sp, #FRAME_FPU
    str     x21, [sp, #FRAME_FPCTX]

    /* FP/SIMD state */
    stp     q0, q1, [sp, #(FRAME_FPU + 0)]
    stp     q2, q3, [sp, #(FRAME_FPU + 32)]
    stp     q4, q5, [sp, #(FRAME_FPU + 64)]
    stp     q6, q7, [sp, #(FRAME_FPU + 96)]
    stp     q8, q9, [sp, #(FRAME_FPU + 128)]
    stp     q10, q11, [sp, #(FRAME_FPU + 160)]
    stp     q12, q13, [sp, #(FRAME_FPU + 192)]
    stp     q14, q15, [sp, #(FRAME_FPU + 224)]
    stp     q16, q17, [sp, #(FRAME_FPU + 256)]
    stp     q18, q19, [sp, #(FRAME_FPU + 288)]
    stp     q20, q21, [sp, #(FRAME_FPU + 320)]
    stp     q22, q23, [sp, #(FRAME_FPU + 352)]
    stp     q24, q25, [sp, #(FRAME_FPU + 384)]
    stp     q26, q27, [sp, #(FRAME_FPU + 416)]
    stp     q28, q29, [sp, #(FRAME_FPU + 448)]
    stp     q30, q31, [sp, #(FRAME_FPU + 480)]
    mrs     x21, fpcr
    str     x21, [sp, #FRAME_FPCR]
    mrs     x21, fpsr
    str     x21, [sp, #FRAME_FPSR]
.endm

/*
 * Restore all registers from the exception frame and return.
 * \to_el0: 1 if returning to EL0, 0 if to EL1
 */
.macro restore_context, to_el0
    /*
     * Mask all exceptions for the duration of the restore. Interrupt
     * handler code may have re-enabled IRQs (a driver calling Enable()
     * issues a nested SC_STI that clears the I bit in this handler's
     * saved SPSR), so this sequence can otherwise be interrupted AFTER
     * spsr_el1/elr_el1 are loaded: the interrupting frame then captures
     * a PC inside this code with IRQs enabled, and restoring that frame
     * erets back into the middle of the restore with sp already popped -
     * a self-sustaining eret treadmill that marches SP_EL1 upward
     * through memory (observed on real Pi 3B+). The eret installs the
     * target context's own PSTATE, so masking here has no lasting effect.
     */
    msr     daifset, #0xf

    /* Restore system registers first using x0 as scratch (cpsr is a
     * 32-bit field; the AArch64 SPSR bits all live in the low word) */
    ldr     w0, [sp, #FRAME_CPSR]
    msr     spsr_el1, x0

    ldr     x0, [sp, #FRAME_PC]
    msr     elr_el1, x0

    .if \to_el0
    ldr     x0, [sp, #FRAME_SP]
    msr     sp_el0, x0
    .endif

    /* Restore FP/SIMD state */
    ldr     x0, [sp, #FRAME_FPCR]
    msr     fpcr, x0
    ldr     x0, [sp, #FRAME_FPSR]
    msr     fpsr, x0
    ldp     q0, q1, [sp, #(FRAME_FPU + 0)]
    ldp     q2, q3, [sp, #(FRAME_FPU + 32)]
    ldp     q4, q5, [sp, #(FRAME_FPU + 64)]
    ldp     q6, q7, [sp, #(FRAME_FPU + 96)]
    ldp     q8, q9, [sp, #(FRAME_FPU + 128)]
    ldp     q10, q11, [sp, #(FRAME_FPU + 160)]
    ldp     q12, q13, [sp, #(FRAME_FPU + 192)]
    ldp     q14, q15, [sp, #(FRAME_FPU + 224)]
    ldp     q16, q17, [sp, #(FRAME_FPU + 256)]
    ldp     q18, q19, [sp, #(FRAME_FPU + 288)]
    ldp     q20, q21, [sp, #(FRAME_FPU + 320)]
    ldp     q22, q23, [sp, #(FRAME_FPU + 352)]
    ldp     q24, q25, [sp, #(FRAME_FPU + 384)]
    ldp     q26, q27, [sp, #(FRAME_FPU + 416)]
    ldp     q28, q29, [sp, #(FRAME_FPU + 448)]
    ldp     q30, q31, [sp, #(FRAME_FPU + 480)]

    /* Restore general purpose registers */
    ldr     x30, [sp, #FRAME_LR]
    ldp     x28, x29, [sp, #224]
    ldp     x26, x27, [sp, #208]
    ldp     x24, x25, [sp, #192]
    ldp     x22, x23, [sp, #176]
    ldp     x20, x21, [sp, #160]
    ldp     x18, x19, [sp, #144]
    ldp     x16, x17, [sp, #128]
    ldp     x14, x15, [sp, #112]
    ldp     x12, x13, [sp, #96]
    ldp     x10, x11, [sp, #80]
    ldp     x8, x9, [sp, #64]
    ldp     x6, x7, [sp, #48]
    ldp     x4, x5, [sp, #32]
    ldp     x2, x3, [sp, #16]
    ldp     x0, x1, [sp, #0]

    add     sp, sp, #FRAME_SIZE
    eret
.endm


/* ================================================================== */
/*  Exception Vector Table                                            */
/*  Must be 2048-byte aligned. Each entry is 128 bytes (32 insns).    */
/* ================================================================== */

.section .intvecs, "ax"
.balign 2048
.globl __aarch64_vectors
__aarch64_vectors:

/* ---- Group 0: Current EL with SP_EL0 ----
 * The OS runs at EL1t (SPSel=0), so its own synchronous traps (SVC), IRQs,
 * FIQs and SErrors arrive through this group. The interrupted context uses
 * SP_EL0 exactly like a lower-EL (EL0) exception, so the SP save/restore is
 * identical -- route to the same handlers (which read/write SP_EL0). The C
 * handlers distinguish the originating EL from the saved SPSR, not the
 * vector slot. */
.balign 128
    b       __el0_sync
.balign 128
    b       __el0_irq
.balign 128
    b       __el0_fiq
.balign 128
    b       __el0_serror

/* ---- Group 1: Current EL with SP_ELx (kernel exceptions) ---- */
.balign 128
    b       __el1_sync
.balign 128
    b       __el1_irq
.balign 128
    b       __el1_fiq
.balign 128
    b       __el1_serror

/* ---- Group 2: Lower EL using AArch64 (user mode exceptions) ---- */
.balign 128
    b       __el0_sync
.balign 128
    b       __el0_irq
.balign 128
    b       __el0_fiq
.balign 128
    b       __el0_serror

/* ---- Group 3: Lower EL using AArch32 (not used) ---- */
.balign 128
    b       __aarch64_halt
.balign 128
    b       __aarch64_halt
.balign 128
    b       __aarch64_halt
.balign 128
    b       __aarch64_halt


/* ================================================================== */
/*  Exception Handlers                                                */
/* ================================================================== */

.text

/*
 * Halt loop - for unhandled vectors
 */
.globl __aarch64_halt
.type __aarch64_halt, %function
__aarch64_halt:
    wfi
    b       __aarch64_halt

/* ---- EL1 (Kernel) Exception Handlers ---- */

.type __el1_sync, %function
__el1_sync:
    save_context 0
    mov     x0, sp
    bl      handle_sync
    restore_context 0

.type __el1_irq, %function
__el1_irq:
    save_context 0
    mov     x0, sp
    bl      handle_irq
    /* No core_ExitInterrupt for EL1 - don't reschedule in kernel mode */
    restore_context 0

.type __el1_fiq, %function
__el1_fiq:
    save_context 0
    mov     x0, sp
    bl      handle_fiq
    restore_context 0

.type __el1_serror, %function
__el1_serror:
    save_context 0
    mov     x0, sp
    bl      handle_serror
    restore_context 0

/* ---- EL0 (User) Exception Handlers ---- */

.type __el0_sync, %function
__el0_sync:
    save_context 1
    mov     x0, sp
    bl      handle_sync
    restore_context 1

.type __el0_irq, %function
__el0_irq:
    save_context 1
    mov     x0, sp
    bl      handle_irq
    /* Check if we need to reschedule when returning to user mode */
    mov     x0, sp
    mov     fp, #0
    bl      core_ExitInterrupt
    restore_context 1

.type __el0_fiq, %function
__el0_fiq:
    save_context 1
    mov     x0, sp
    bl      handle_fiq
    restore_context 1

.type __el0_serror, %function
__el0_serror:
    save_context 1
    mov     x0, sp
    bl      handle_serror
    restore_context 1

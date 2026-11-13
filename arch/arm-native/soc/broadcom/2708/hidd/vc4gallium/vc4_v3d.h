/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    VideoCore IV V3D register definitions
    Reference: BCM2835 ARM Peripherals + VideoCoreIV-AG100-R
*/

#ifndef _VC4_V3D_H
#define _VC4_V3D_H

#include <exec/types.h>

extern IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase
#include <hardware/bcm2708.h>

/* V3D register offsets from V3D_BASE */
#define V3D_IDENT0      0x000   /* V3D Identification 0 (V3D block identity) */
#define V3D_IDENT1      0x004   /* V3D Identification 1 (V3D configuration A) */
#define V3D_IDENT2      0x008   /* V3D Identification 2 (V3D configuration B) */
#define V3D_SCRATCH     0x010   /* Scratch register */

/* Binning/Rendering pipeline control */
#define V3D_CT0CS       0x100   /* Control List Executor Thread 0 Control and Status */
#define V3D_CT1CS       0x104   /* Control List Executor Thread 1 Control and Status */
#define V3D_CT0EA       0x108   /* Control List Executor Thread 0 End Address */
#define V3D_CT1EA       0x10c   /* Control List Executor Thread 1 End Address */
#define V3D_CT0CA       0x110   /* Control List Executor Thread 0 Current Address */
#define V3D_CT1CA       0x114   /* Control List Executor Thread 1 Current Address */
#define V3D_CT00RA0     0x118   /* Control List Executor Thread 0 Return Address */
#define V3D_CT01RA0     0x11c   /* Control List Executor Thread 1 Return Address */
#define V3D_CT0LC       0x120   /* Control List Executor Thread 0 List Counter */
#define V3D_CT1LC       0x124   /* Control List Executor Thread 1 List Counter */
#define V3D_CT0PC       0x128   /* Control List Executor Thread 0 Primitive List Counter */
#define V3D_CT1PC       0x12c   /* Control List Executor Thread 1 Primitive List Counter */

/* V3D Pipeline state */
#define V3D_PCS         0x130   /* V3D Pipeline Control and Status */
#define V3D_BFC         0x134   /* Binning Mode Flush Count */
#define V3D_RFC         0x138   /* Rendering Mode Frame Count */

/* Binning Memory Pool */
#define V3D_BPCA        0x300   /* Current Pool Address */
#define V3D_BPCS        0x304   /* Current Pool Size */
#define V3D_BPOA        0x308   /* Overspill Pool Address */
#define V3D_BPOS        0x30c   /* Overspill Pool Size */

/* L2 Cache Control */
#define V3D_L2CACTL     0x020   /* L2 Cache Control */
/* Slice cache control. Independent 4-bit fields, one bit per slice; writing
 * 0xf in a field invalidates that cache across all 4 slices. */
#define V3D_SLCACTL     0x024   /* Slice Cache Control */
#define V3D_SLCACTL_ICC_SHIFT   0   /* QPU instruction cache */
#define V3D_SLCACTL_UCC_SHIFT   8   /* Uniforms cache */
#define V3D_SLCACTL_T0CC_SHIFT  16  /* Texture 0 cache */
#define V3D_SLCACTL_T1CC_SHIFT  24  /* Texture 1 cache */
#define V3D_SLCACTL_ALL         0x0f0f0f0f  /* Clear all four slice caches */

/* Slices / QPU Scheduler */
#define V3D_SQRSV0      0x410   /* Reserve QPUs 0-7 */
#define V3D_SQRSV1      0x414   /* Reserve QPUs 8-15 */
#define V3D_SQCNTL      0x418   /* QPU Scheduler Control */
#define V3D_SRQPC       0x430   /* QPU User Program Request Queue Program Counter */
#define V3D_SRQUA       0x434   /* QPU User Program Request Queue Uniforms Address */
#define V3D_SRQUL       0x438   /* QPU User Program Request Queue Uniforms Length */
#define V3D_SRQCS       0x43c   /* QPU User Program Request Queue Control and Status */

/* VPM (Vertex Pipe Memory) */
#define V3D_VPACNTL     0x500   /* VPM Allocator Control */
#define V3D_VPMBASE     0x504   /* VPM Base (User) Memory Reservation */

/* Performance Counters */
#define V3D_PCTRC       0x670   /* Performance Counter Clear */
#define V3D_PCTRE       0x674   /* Performance Counter Enable */
#define V3D_PCTR(n)     (0x680 + (n) * 0x08)   /* Performance Counter Count n */
#define V3D_PCTRS(n)    (0x684 + (n) * 0x08)   /* Performance Counter Mapping n */

/* Interrupt Control. */
#define V3D_INTCTL      0x030   /* Interrupt Control / Status (W1C) */
#define V3D_INTENA      0x034   /* Interrupt Enable Set */
#define V3D_INTDIS      0x038   /* Interrupt Enable Clear */

/* Error / Debug */
#define V3D_ERRSTAT     0xf20   /* Miscellaneous Error Signals (active bits) */
#define V3D_DBGE        0xf00   /* PSE Error Signals */
#define V3D_FDBGO       0xf04   /* FEP Overrun Error Signals */
#define V3D_FDBGB       0xf08   /* FEP Interface Ready and Stall Signals, FEP Busy Signals */
#define V3D_FDBGR       0xf0c   /* FEP Internal Ready Signals */
#define V3D_FDBGS       0xf10   /* FEP Internal Stall Input Signals */

/* V3D_CTnCS bits */
#define V3D_CTCS_CTRSTA    (1 << 15)  /* Reset bit, write 1 to force thread to stop */
#define V3D_CTCS_CTSEMA    (1 << 12)  /* Control Thread interacted with semaphore */
#define V3D_CTCS_CTRTSD    (1 << 8)   /* Control Thread Return-To-Subroutine-Delay */
#define V3D_CTCS_CTRUN     (1 << 5)   /* Control Thread is running */
#define V3D_CTCS_CTSUBS    (1 << 4)   /* Control Thread executing subroutine */
#define V3D_CTCS_CTERR     (1 << 3)   /* Control Thread had error (read-only) */
#define V3D_CTCS_CTMODE    (1 << 0)   /* Control Thread mode bit */

/* V3D_INTCTL / V3D_INTENA / V3D_INTDIS bits */
#define V3D_INT_FLDONE     (1 << 1)   /* Binning Mode Flush Done */
#define V3D_INT_FRDONE     (1 << 0)   /* Rendering Mode Frame Done */
#define V3D_INT_OUTOMEM    (1 << 2)   /* Binner Ran Out of Memory */

/* V3D_L2CACTL bits */
#define V3D_L2CACTL_L2CENA (1 << 0)   /* L2 Cache Enable */
#define V3D_L2CACTL_L2CCLR (1 << 2)   /* L2 Cache Clear */

/* V3D_PCS bits */
#define V3D_PCS_BMACTIVE   (1 << 0)   /* Binning mode active */
#define V3D_PCS_BMBUSY     (1 << 1)   /* Binning mode busy */
#define V3D_PCS_RMACTIVE   (1 << 2)   /* Rendering mode active */
#define V3D_PCS_RMBUSY     (1 << 3)   /* Rendering mode busy */
#define V3D_PCS_BMOOM      (1 << 8)   /* Binning mode out of memory */

/* IDENT0 fields */
#define V3D_IDENT0_VER_SHIFT    24
#define V3D_IDENT0_VER_MASK     0xFF000000

#define V3D_READ(state, reg)        ((state)->v3d_regs[(reg) / 4])
#define V3D_WRITE(state, reg, val)  ((state)->v3d_regs[(reg) / 4] = (val))

/* CPU address of a VideoCore buffer -> the GPU's uncached bus alias.
 * The VC region is identity-mapped below 1GB by the raspi bootstrap (arm
 * and aarch64 alike), so the low 32 bits are the physical address; the
 * IPTR step is what keeps 64-bit builds from warning about the narrowing
 * that is intended here. VideoCore itself is 32-bit whatever the CPU
 * mode, so bus addresses stay ULONG. */
#define GPU_BUS_ADDR(x) (0xC0000000 | (ULONG)(IPTR)(x))

BOOL vc4_v3d_init(struct vc4_v3d_state *v3d);
void vc4_v3d_service_interrupts(struct vc4_v3d_state *v3d);
void vc4_v3d_kick_pending_render(struct vc4_v3d_state *v3d, const char *reason);
void vc4_v3d_advance_counters(struct vc4_v3d_state *v3d);

#endif /* _VC4_V3D_H */

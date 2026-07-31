/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: CPU probing and context sizing, 64bit RISC-V version.
*/

#include <aros/symbolsets.h>
#include <exec/types.h>

#include <aros/riscv64/cpucontext.h>
#include <asm/cpu.h>

#include "kernel_base.h"
#include "kernel_cpu.h"

/*
 * Bytes per vector register (the vlenb CSR), or 0 when the vector
 * extension is not implemented. Probed once at boot; KrnCreateContext()
 * uses it to lay out the per-task vector save area.
 */
unsigned long __riscv_vlenb;

/*
 * The job of this function is to probe the CPU and set up kb_ContextFlags
 * and kb_ContextSize.
 * kb_ContextFlags is whatever needs to be passed to KrnCreateContext() in
 * order to create a right thing. kb_ContextSize is total length of our
 * context area (including FPU data, vector data and private data). It is
 * needed for complete context save/restore during Exec exceptions
 * processing.
 */

static int cpu_Init(struct KernelBase *KernelBase)
{
    /*
     * Runtime-detect the vector extension: sstatus.VS reads as a
     * hardwired zero when V is not implemented, so enable it and see if
     * the write sticks. The vector CSRs may only be accessed while VS is
     * enabled, so vlenb is read before switching VS back off.
     */
    csr_set(sstatus, SSTATUS_VS_INITIAL);
    if (csr_read(sstatus) & SSTATUS_VS)
    {
        __riscv_vlenb = csr_read(0xC22 /* CSR_VLENB */);
        csr_clear(sstatus, SSTATUS_VS);
    }

    /*
     * The ExceptionContext, the FPU block and (when present) the vector
     * block are allocated as one chunk; KrnCreateContext() points
     * fpuContext/vecContext at the 16-byte aligned tails.
     */
    KernelBase->kb_ContextSize = sizeof(struct ExceptionContext) + 15
                               + sizeof(struct FpuContext);
    if (__riscv_vlenb)
    {
        KernelBase->kb_ContextSize += 15 + sizeof(struct VectorContext)
                                    + (32 * __riscv_vlenb);
    }

    return TRUE;
}

ADD2INITLIB(cpu_Init, 5);

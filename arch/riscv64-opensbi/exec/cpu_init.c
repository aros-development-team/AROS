/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: What this machine can do, and the code that uses it.
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include <defines/exec_LVO.h>

#include "kernel_sbi.h"

/*
    RISC-V is a base with extensions bolted on, and which ones are there
    is not known until the machine is running - misa cannot be read from
    supervisor mode, so nothing can be settled at build time either.
    Naming an extension in the code would only narrow the machines the
    image will start on.

    So exec ships the generic version of anything that could use one (see
    arch/riscv64-all/exec), and what is found here replaces it.

    Instruction fetch is the first case. Stores reach it only when
    something says so, and this platform can always ask the SEE: the call
    needs no extension of ours, is mandatory in SBI v0.1 so it is always
    answered, and reaches every hart rather than only the one asking -
    which is what more than one hart will want anyway. Zifencei would do
    it locally and quicker, but only where the machine admits to having
    it, and this one does not advertise it even though it does.
*/

AROS_LH3(void, CacheClearE_SBI,
    AROS_LHA(APTR,  address, A0),
    AROS_LHA(IPTR,  length,  D0),
    AROS_LHA(ULONG, caches,  D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    __asm__ __volatile__ ("fence rw, rw" ::: "memory");

    if (caches & (CACRF_ClearI | CACRF_ClearD | CACRF_InvalidateD))
        sbi_remote_fence_i();

    AROS_LIBFUNC_EXIT
}

AROS_LH0(void, CacheClearU_SBI,
    struct ExecBase *, SysBase, 106, Exec)
{
    AROS_LIBFUNC_INIT

    __asm__ __volatile__ ("fence rw, rw" ::: "memory");
    sbi_remote_fence_i();

    AROS_LIBFUNC_EXIT
}

static int cpu_Init(struct ExecBase *SysBase)
{
    if (sbi_have_remote_fence_i())
    {
        D(bug("[Exec] riscv64: instruction fetch reconciled through SBI\n"));

        SetFunction(&SysBase->LibNode, -LVOCacheClearE * LIB_VECTSIZE,
                    AROS_SLIB_ENTRY(CacheClearE_SBI, Exec, LVOCacheClearE));
        SetFunction(&SysBase->LibNode, -LVOCacheClearU * LIB_VECTSIZE,
                    AROS_SLIB_ENTRY(CacheClearU_SBI, Exec, LVOCacheClearU));
    }
    D(else bug("[Exec] riscv64: nothing offered to reconcile instruction"
               " fetch - assuming it needs none\n"));

    return TRUE;
}

ADD2INITLIB(cpu_Init, 0);

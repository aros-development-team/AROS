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
#include <aros/kernel.h>
#include <proto/kernel.h>

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

/*
    Data caches are the second case. Devices on a coherent path need no
    help, but a transfer outside coherency (a device using the no-snoop
    attribute, or a port wired past the cache hierarchy) only works if
    the CPU can push and discard its own lines. Zicbom is the extension
    for that; whether it exists is read from the device tree, and these
    replace the do-nothing defaults when it does.

    The assembler may be older than the extension, so the instructions
    are spelled out: cbo.clean/flush are MISC-MEM (0x0f) func3=2 with
    the operation in the immediate. The block size is uniformly 64 on
    hardware advertising the extension (riscv,cbom-block-size).
*/
#define CBO_BLOCK   64

static inline void cbo_clean_range(APTR address, IPTR length)
{
    IPTR p = (IPTR)address & ~(IPTR)(CBO_BLOCK - 1);
    IPTR end = (IPTR)address + length;

    __asm__ __volatile__ ("fence rw, rw" ::: "memory");
    for (; p < end; p += CBO_BLOCK)
        __asm__ __volatile__ (".insn i 0x0f, 2, x0, %0, 1" :: "r"(p) : "memory");
    __asm__ __volatile__ ("fence rw, rw" ::: "memory");
}

static inline void cbo_flush_range(APTR address, IPTR length)
{
    IPTR p = (IPTR)address & ~(IPTR)(CBO_BLOCK - 1);
    IPTR end = (IPTR)address + length;

    __asm__ __volatile__ ("fence rw, rw" ::: "memory");
    for (; p < end; p += CBO_BLOCK)
        __asm__ __volatile__ (".insn i 0x0f, 2, x0, %0, 2" :: "r"(p) : "memory");
    __asm__ __volatile__ ("fence rw, rw" ::: "memory");
}

AROS_LH3(APTR, CachePreDMA_Zicbom,
    AROS_LHA(APTR,    address, A0),
    AROS_LHA(ULONG *, length,  A1),
    AROS_LHA(ULONG,   flags,   D0),
    struct ExecBase *, SysBase, 127, Exec)
{
    AROS_LIBFUNC_INIT

    /*
     * Device reading: push the CPU's writes out so it sees them.
     * Device writing: flush (writeback and discard), so no dirty line
     * is evicted over its data afterwards.
     */
    if (flags & DMA_ReadFromRAM)
        cbo_clean_range(address, *length);
    else
        cbo_flush_range(address, *length);

    return address;

    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, CachePostDMA_Zicbom,
    AROS_LHA(APTR,    address, A0),
    AROS_LHA(ULONG *, length,  A1),
    AROS_LHA(ULONG,   flags,   D0),
    struct ExecBase *, SysBase, 128, Exec)
{
    AROS_LIBFUNC_INIT

    /*
     * The device wrote memory: discard whatever the CPU still holds.
     * Flush rather than invalidate - a shared edge line with unrelated
     * dirty data survives that way, and clean lines just drop.
     */
    if (!(flags & DMA_ReadFromRAM))
        cbo_flush_range(address, *length);

    AROS_LIBFUNC_EXIT
}

/*
 * Whether the machine has Zicbom, from the device tree: the extension
 * name appears in the cpu nodes' riscv,isa-extensions string list.
 * A plain byte scan of the blob is enough - the name is distinctive -
 * and spares this early code a tree walk.
 */
static BOOL cpu_HasZicbom(struct ExecBase *SysBase)
{
    APTR KernelBase = OpenResource("kernel.resource");
    const struct TagItem *tag;
    const UBYTE *dtb = NULL;
    ULONG size, i;

    if (!KernelBase)
        return FALSE;

    for (tag = KrnGetBootInfo(); tag && tag->ti_Tag != TAG_DONE; tag++)
    {
        if (tag->ti_Tag == TAG_MORE)
        {
            tag = (const struct TagItem *)tag->ti_Data;
            if (!tag)
                break;
            continue;
        }
        if (tag->ti_Tag == KRN_FlattenedDeviceTree)
        {
            dtb = (const UBYTE *)tag->ti_Data;
            break;
        }
    }

    if (!dtb)
        return FALSE;

    /* totalsize, big-endian, at offset 4 */
    size = ((ULONG)dtb[4] << 24) | ((ULONG)dtb[5] << 16) |
           ((ULONG)dtb[6] << 8) | dtb[7];
    if (size < 8 || size > (4UL << 20))
        return FALSE;

    for (i = 0; i + 6 <= size; i++)
    {
        if (dtb[i] == 'z' && dtb[i + 1] == 'i' && dtb[i + 2] == 'c' &&
            dtb[i + 3] == 'b' && dtb[i + 4] == 'o' && dtb[i + 5] == 'm')
            return TRUE;
    }
    return FALSE;
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

    if (cpu_HasZicbom(SysBase))
    {
        D(bug("[Exec] riscv64: Zicbom data cache maintenance enabled\n"));

        SetFunction(&SysBase->LibNode, -LVOCachePreDMA * LIB_VECTSIZE,
                    AROS_SLIB_ENTRY(CachePreDMA_Zicbom, Exec, LVOCachePreDMA));
        SetFunction(&SysBase->LibNode, -LVOCachePostDMA * LIB_VECTSIZE,
                    AROS_SLIB_ENTRY(CachePostDMA_Zicbom, Exec, LVOCachePostDMA));
    }

    return TRUE;
}

ADD2INITLIB(cpu_Init, 0);

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <asm/cpu.h>
#include <exec/memory.h>
#include <string.h>
#include <proto/exec.h>
#include <exec_platform.h>       /* IDNESTCOUNT_GET */

#include "kernel_base.h"
#include "kernel_intern.h"

#if defined(__AROSEXEC_SMP__)
#include <utility/hooks.h>
#include <proto/kernel.h>
#include "kernel_ipi.h"
#endif

#define MAP_2M_SHIFT 21
#define MAP_2M_SIZE  (1UL << MAP_2M_SHIFT)
#define MAP_2M_MASK  (MAP_2M_SIZE - 1)
#define MAP_4K_MASK  ((IPTR)PAGE_SIZE - 1)

extern int Kernel_13_KrnIsSuper(void);

struct pt_pool {
    IPTR *raw;
    APTR *aligned;
    int n;
    int i;
};

static APTR alloc_page_table(struct pt_pool *pool)
{
    if (pool && pool->i < pool->n)
        return pool->aligned[pool->i++];

    return NULL;
}

static struct pt_pool *pt_pool_create(int count)
{
    struct pt_pool *p;
    int i;

    p = AllocMem(sizeof(*p) + sizeof(IPTR) * count + sizeof(APTR) * count,
                 MEMF_PUBLIC | MEMF_CLEAR);
    if (!p)
        return NULL;

    p->raw = (IPTR *)((UBYTE *)p + sizeof(*p));
    p->aligned = (APTR *)(p->raw + count);
    p->n = 0;
    p->i = 0;

    for (i = 0; i < count; i++) {
        IPTR raw = (IPTR)AllocMem(PAGE_SIZE * 2, MEMF_PUBLIC | MEMF_CLEAR);

        if (!raw)
            break;

        p->raw[p->n] = raw;
        p->aligned[p->n] = (APTR)((raw + PAGE_SIZE - 1) & ~(IPTR)(PAGE_SIZE - 1));
        p->n++;
    }

    if (p->n < count) {
        /* Could not fill the pool: free and fail. */
        for (i = 0; i < p->n; i++)
            FreeMem((APTR)p->raw[i], PAGE_SIZE * 2);
        FreeMem(p, sizeof(*p) + sizeof(IPTR) * count + sizeof(APTR) * count);
        return NULL;
    }

    return p;
}

static void pt_pool_destroy(struct pt_pool *p)
{
    int i;

    if (!p)
        return;

    for (i = p->i; i < p->n; i++)
        FreeMem((APTR)p->raw[i], PAGE_SIZE * 2);

    FreeMem(p, sizeof(*p) + sizeof(IPTR) * p->n + sizeof(APTR) * p->n);
}

static APTR table_addr(ULONG base_low, ULONG base_high)
{
    return (APTR)(((IPTR)base_low << 12) | ((IPTR)base_high << 32));
}

static void set_pml4e(struct PML4E *entry, APTR table)
{
    struct PML4E tmp = { 0 };
    uint64_t val;

    tmp.p = 1;
    tmp.rw = 1;
    tmp.us = 1;
    tmp.base_low = (IPTR)table >> 12;
    tmp.base_high = ((IPTR)table >> 32) & 0x000FFFFF;
    memcpy(&val, &tmp, sizeof(val));
    __atomic_store_n((volatile uint64_t *)(void *)entry, val, __ATOMIC_RELAXED);
}

static void set_pdpe(struct PDPE *entry, APTR table)
{
    struct PDPE tmp = { 0 };
    uint64_t val;

    tmp.p = 1;
    tmp.rw = 1;
    tmp.us = 1;
    tmp.base_low = (IPTR)table >> 12;
    tmp.base_high = ((IPTR)table >> 32) & 0x000FFFFF;
    memcpy(&val, &tmp, sizeof(val));
    __atomic_store_n((volatile uint64_t *)(void *)entry, val, __ATOMIC_RELAXED);
}

static void set_pde_table(struct PDE4K *entry, APTR table)
{
    struct PDE4K tmp = { 0 };
    uint64_t val;

    tmp.p = 1;
    tmp.rw = 1;
    tmp.us = 1;
    tmp.base_low = (IPTR)table >> 12;
    tmp.base_high = ((IPTR)table >> 32) & 0x000FFFFF;
    memcpy(&val, &tmp, sizeof(val));
    __atomic_store_n((volatile uint64_t *)(void *)entry, val, __ATOMIC_RELAXED);
}

static void set_pde_2m(struct PDE2M *entry, IPTR phys, KRN_MapAttr flags)
{
    struct PDE2M tmp = { 0 };
    uint64_t val;

    /* Built then stored whole: this replaces live mappings, so a reader
       between two field stores could see p=1 over a half-written base. */
    tmp.p = (flags & (MAP_Readable | MAP_Writable | MAP_Executable)) ? 1 : 0;
    tmp.rw = (flags & MAP_Writable) ? 1 : 0;
    tmp.us = 1;
    tmp.pwt = (flags & MAP_WriteThrough) ? 1 : 0;
    tmp.pcd = (flags & MAP_CacheInhibit) ? 1 : 0;
    tmp.ps = 1;
    tmp.base_low = phys >> 13;
    tmp.base_high = (phys >> 32) & 0x000FFFFF;
    memcpy(&val, &tmp, sizeof(val));
    __atomic_store_n((volatile uint64_t *)(void *)entry, val, __ATOMIC_RELAXED);
}

static void set_pte(struct PTE *entry, IPTR phys, KRN_MapAttr flags)
{
    struct PTE tmp = { 0 };
    uint64_t val;

    tmp.p = (flags & (MAP_Readable | MAP_Writable | MAP_Executable)) ? 1 : 0;
    tmp.rw = (flags & MAP_Writable) ? 1 : 0;
    tmp.us = 1;
    tmp.pwt = (flags & MAP_WriteThrough) ? 1 : 0;
    tmp.pcd = (flags & MAP_CacheInhibit) ? 1 : 0;
    tmp.base_low = phys >> 12;
    tmp.base_high = (phys >> 32) & 0x000FFFFF;
    memcpy(&val, &tmp, sizeof(val));
    __atomic_store_n((volatile uint64_t *)(void *)entry, val, __ATOMIC_RELAXED);
}

/* Page directory for vaddr, creating the levels above it if create. NULL if absent. */
static struct PDE2M *pd_for(IPTR vaddr, BOOL create, struct pt_pool *pool)
{
    struct PML4E *pml4 = __KernBootPrivate->MMU.mmu_PML4;
    ULONG pml4_off = (vaddr >> 39) & 0x1ff;
    ULONG pdpe_off = (vaddr >> 30) & 0x1ff;
    struct PDPE *pdpe;
    struct PDE2M *pde;

    if (!pml4)
        return NULL;

    if (!pml4[pml4_off].p) {
        if (!create || !(pdpe = alloc_page_table(pool)))
            return NULL;
        set_pml4e(&pml4[pml4_off], pdpe);
    } else {
        pdpe = table_addr(pml4[pml4_off].base_low, pml4[pml4_off].base_high);
    }

    if (!pdpe[pdpe_off].p) {
        if (!create || !(pde = alloc_page_table(pool)))
            return NULL;
        set_pdpe(&pdpe[pdpe_off], pde);
    } else {
        pde = table_addr(pdpe[pdpe_off].base_low, pdpe[pdpe_off].base_high);
    }

    return pde;
}

/* 4 KiB table for *pde, splitting a 2 MiB mapping there first if create. */
static struct PTE *pt_for(struct PDE2M *pde, BOOL create, struct pt_pool *pool)
{
    struct PTE *pt;
    int i;

    if (pde->p && !pde->ps)
        return table_addr(((struct PDE4K *)pde)->base_low,
            ((struct PDE4K *)pde)->base_high);

    if (!create || !(pt = alloc_page_table(pool)))
        return NULL;

    if (pde->p) {
        IPTR base = ((IPTR)pde->base_low << 13) | ((IPTR)pde->base_high << 32);

        for (i = 0; i < 512; i++) {
            pt[i].p = 1;
            pt[i].rw = pde->rw;
            pt[i].us = pde->us;
            pt[i].pwt = pde->pwt;
            pt[i].pcd = pde->pcd;
            pt[i].pat = pde->pat;
            pt[i].g = pde->g;
            pt[i].nx = pde->nx;
            pt[i].base_low = (base + ((IPTR)i << 12)) >> 12;
            pt[i].base_high = ((base + ((IPTR)i << 12)) >> 32) & 0x000FFFFF;
        }
    }

    set_pde_table((struct PDE4K *)pde, pt);
    return pt;
}

#if defined(__AROSEXEC_SMP__)
AROS_UFH3(static void, flush_tlb_hook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, object, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    /* A cr3 reload does not evict global entries. It is enough only because
       no page-table entry sets G; adding one means using invlpg here. */
    wrcr(cr3, rdcr(cr3));

    AROS_USERFUNC_EXIT
}
#endif

/*
 * Ask every other CPU to reload cr3. It must be called with interrupts able
 * to service the reply: core_DoCallIPI() spins on ih_SyncLock until the hook
 * has run everywhere, so two CPUs both shooting down with interrupts masked
 * would each wait for an IPI the other cannot take. async=1 avoids the wait,
 * at the cost of the shootdown completing later.
 */
void core_FlushOtherCPUTLB(int async, struct KernelBase *KernelBase)
{
#if defined(__AROSEXEC_SMP__)
    if (KrnGetCPUCount() > 1) {
        struct Hook hook = { { 0 }, (APTR)flush_tlb_hook, NULL, KernelBase };

        core_DoCallIPI(&hook, (void *)TASKAFFINITY_ALL_BUT_SELF, async, 0, NULL,
            KernelBase);
    }
#else
    (void)async;
    (void)KernelBase;
#endif
}

/* Safe inside the supervisor window: needs nothing from the interrupt state. */
void core_FlushOwnTLB(void)
{
    wrcr(cr3, __KernBootPrivate->MMU.mmu_PML4);
}

/*
 * Serialises every edit of the shared page tables, including core_ProtPage()'s.
 * Held over the table writes only: core_FlushOtherCPUTLB() must not be called
 * with it held, or its sync wait deadlocks against a CPU waiting for this lock.
 *
 * KrnSpinInit() only zeroes the lock, so a statically zero-initialised
 * spinlock_t is already initialised. There is no init call here, and so no
 * race between two CPUs running one.
 */
#if defined(__AROSEXEC_SMP__)
static spinlock_t ptbl_lock;

/*
 * Always taken, also before the APs are up (core_ProtPage() is reached from
 * kernel_startup.c before PlatformPostInit()): uncontended, it costs nothing.
 * It goes through the kernel's own entry points, not the LVO stubs, so it
 * needs no library base.
 */
void core_PageTableLock(void)
{
    Kernel_52_KrnSpinLock(&ptbl_lock, NULL, SPINLOCK_MODE_WRITE, NULL);
}

void core_PageTableUnlock(void)
{
    Kernel_53_KrnSpinUnLock(&ptbl_lock, NULL);
}
#else
/* One CPU, nothing to serialise. */
void core_PageTableLock(void)
{
}

void core_PageTableUnlock(void)
{
}
#endif

/*
 * How many new tables mapping [vaddr, vend) (or unmapping it, if unmap) needs,
 * counted from the live tables without the lock. Tables are only ever added,
 * never freed, so a concurrent edit can only lower the real need and the count
 * is an upper bound. For a range whose tables already exist it is 0.
 */
static int pt_needed(IPTR vaddr, IPTR vend, IPTR paddr, BOOL unmap)
{
    struct PML4E *pml4 = __KernBootPrivate->MMU.mmu_PML4;
    IPTR last_pml4 = ~(IPTR)0, last_pdpe = ~(IPTR)0;
    int need = 0;

    if (!pml4)
        return 0;

    while (vaddr < vend) {
        IPTR next = (vaddr + MAP_2M_SIZE) & ~(IPTR)MAP_2M_MASK;
        ULONG pml4_off = (vaddr >> 39) & 0x1ff;
        ULONG pdpe_off = (vaddr >> 30) & 0x1ff;
        BOOL whole = !(vaddr & MAP_2M_MASK) && vend - vaddr >= MAP_2M_SIZE;
        struct PDE2M *pde = NULL;

        if (pml4[pml4_off].p) {
            struct PDPE *pdpe = table_addr(pml4[pml4_off].base_low, pml4[pml4_off].base_high);

            if (pdpe[pdpe_off].p) {
                struct PDE2M *pd = table_addr(pdpe[pdpe_off].base_low, pdpe[pdpe_off].base_high);

                pde = &pd[(vaddr >> 21) & 0x1ff];
            }
        }

        if (unmap) {
            /* Unmapping only allocates to split a partly covered 2 MiB entry. */
            if (pde && pde->p && pde->ps && !whole)
                need++;
        } else {
            if (!pml4[pml4_off].p && (vaddr >> 39) != last_pml4) {
                need++;
                last_pml4 = vaddr >> 39;
            }
            if (!pde && (vaddr >> 30) != last_pdpe) {
                need++;
                last_pdpe = vaddr >> 30;
            }
            /* A 4 KiB table is needed unless the run is a whole aligned
               2 MiB one, or one is already there. */
            if (!(whole && !(paddr & MAP_2M_MASK)) && !(pde && pde->p && !pde->ps))
                need++;
        }

        paddr += next - vaddr;
        vaddr = next;
    }

    return need;
}

/*
 * Allocate the tables a map or unmap of [vaddr, vend) can need. Called before
 * Forbid()/SuperState(), so the allocation runs with the caller's interrupt
 * state. *pool stays NULL when nothing is needed. -1 if allocation failed.
 */
int core_PTPoolPrepare(IPTR vaddr, IPTR vend, IPTR paddr, BOOL unmap, struct pt_pool **pool)
{
    int need = pt_needed(vaddr, vend, paddr, unmap);

    *pool = NULL;
    if (!need)
        return 0;

    *pool = pt_pool_create(need);
    return *pool ? 0 : -1;
}

void core_PTPoolDispose(struct pt_pool *pool)
{
    pt_pool_destroy(pool);
}

static int map_global(IPTR vaddr, IPTR paddr, IPTR length, KRN_MapAttr flags,
    struct pt_pool *pool)
{
    IPTR vend = vaddr + length;
    int ret = 0;

    core_PageTableLock();

    while (vaddr < vend) {
        struct PDE2M *pd = pd_for(vaddr, TRUE, pool);
        struct PDE2M *pde;
        struct PTE *pt;

        if (!pd) {
            ret = -1;
            break;
        }
        pde = &pd[(vaddr >> 21) & 0x1ff];

        /* A whole aligned 2 MiB run, and no 4 KiB table to discard: one entry. */
        if (!((vaddr | paddr) & MAP_2M_MASK) && vend - vaddr >= MAP_2M_SIZE &&
            !(pde->p && !pde->ps)) {
            set_pde_2m(pde, paddr, flags);
            asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
            vaddr += MAP_2M_SIZE;
            paddr += MAP_2M_SIZE;
            continue;
        }

        if (!(pt = pt_for(pde, TRUE, pool))) {
            ret = -1;
            break;
        }
        set_pte(&pt[(vaddr >> 12) & 0x1ff], paddr, flags);
        asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
        vaddr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }

    core_PageTableUnlock();

    return ret;
}

/* KrnUnmapGlobal(): the caller is in supervisor state (see unmapglobal.c). */
int core_UnmapGlobal(IPTR vaddr, IPTR vend, struct pt_pool *pool)
{
    int ret = 0;

    core_PageTableLock();

    while (vaddr < vend) {
        struct PDE2M *pd = pd_for(vaddr, FALSE, pool);
        struct PDE2M *pde;
        struct PTE *pt;

        if (!pd) {
            /* nothing mapped in this 1 GiB slot */
            vaddr = (vaddr + (1UL << 30)) & ~((1UL << 30) - 1);
            continue;
        }
        pde = &pd[(vaddr >> 21) & 0x1ff];

        if (!pde->p) {
            vaddr = (vaddr + MAP_2M_SIZE) & ~(IPTR)MAP_2M_MASK;
            continue;
        }

        if (pde->ps && !(vaddr & MAP_2M_MASK) && vend - vaddr >= MAP_2M_SIZE) {
            __atomic_store_n((volatile uint64_t *)(void *)pde, 0ULL, __ATOMIC_RELAXED);
            asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
            vaddr += MAP_2M_SIZE;
            continue;
        }

        if (!(pt = pt_for(pde, TRUE, pool))) {
            ret = -1;
            break;
        }
        __atomic_store_n((volatile uint64_t *)(void *)&pt[(vaddr >> 12) & 0x1ff], 0ULL, __ATOMIC_RELAXED);
        asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
        vaddr += PAGE_SIZE;
    }

    core_PageTableUnlock();

    return ret;
}

/* See rom/kernel/mapglobal.c for documentation. */
AROS_LH4I(int, KrnMapGlobal,
    AROS_LHA(void *, virt, A0),
    AROS_LHA(void *, phys, A1),
    AROS_LHA(uint32_t, length, D0),
    AROS_LHA(KRN_MapAttr, flags, D1),
    struct KernelBase *, KernelBase, 16, Kernel)
{
    AROS_LIBFUNC_INIT

    IPTR vaddr = (IPTR)virt;
    IPTR paddr = phys ? (IPTR)phys : vaddr;
    IPTR maplen;
    struct pt_pool *pool;
    APTR ssp = NULL;
    int ret;

    /* Both addresses carry the same offset inside the page, so they must agree
       below 4 KiB. */
    if (!length || ((vaddr ^ paddr) & MAP_4K_MASK))
        return 0;

    /* An IPTR local: rounding back into the uint32_t parameter wraps to zero near
       4 GiB, and the call then maps nothing and reports success. */
    maplen = ((vaddr & MAP_4K_MASK) + (IPTR)length + MAP_4K_MASK) & ~MAP_4K_MASK;
    vaddr &= ~MAP_4K_MASK;
    paddr &= ~MAP_4K_MASK;

    /* Before Forbid()/SuperState(): AllocMem() must not run with IF clear. */
    if (core_PTPoolPrepare(vaddr, vaddr + maplen, paddr, FALSE, &pool))
        return 0;

    if (!Kernel_13_KrnIsSuper()) {
        /*
         * Forbid() covers the whole window from here to the shootdown. After
         * UserState() the task is preemptible again, and an IPI_RESCHEDULE
         * that arrives while the CPU is in user mode and not in forbid state
         * makes the receiving CPU switch tasks (see kernel_ipi.c). This task
         * could then be dispatched on another CPU before core_DoCallIPI()
         * evaluates TASKAFFINITY_ALL_BUT_SELF, and the CPU it left would
         * never have its TLB flushed. Forbid() defers that switch instead of
         * performing it, and it leaves the interrupt state alone, so the
         * sync wait below can still service an incoming IPI.
         */
        Forbid();
        ssp = SuperState();
        if (!ssp) {
            Permit();
            core_PTPoolDispose(pool);
            return 0;
        }
    }

    ret = map_global(vaddr, paddr, maplen, flags, pool);

    /* Local cr3 first, inside the supervisor window where the interrupt state
       does not matter, so the shootdown below is always ordered after it. */
    core_FlushOwnTLB();

    if (ssp)
        UserState(ssp);

    /*
     * The IPI must wait for UserState(). SuperState() enters through an
     * interrupt gate and core_Supervisor returns with jmpq *%rdi, not iretq,
     * so IF stays clear for the whole window (Enable() skips KrnSti() while
     * KrnIsSuper()); a sync IPI from in there spins on ih_SyncLock unable to
     * take the reply it is waiting for.
     *
     * After UserState() the interrupt flag is back to what the caller had,
     * so the sync form works there - unless the caller is inside Disable(),
     * where interrupts stay off (IDNESTCOUNT_GET >= 0), or unless the caller
     * was already in supervisor state (ssp == NULL), where this code never
     * entered or left a supervisor window and so never restored the
     * interrupt flag itself and cannot know that it is set. Both take the
     * async form rather than a refusal - BAR mapping runs under Forbid(),
     * and failing there would break it.
     */
    core_FlushOtherCPUTLB((ssp == NULL) || (IDNESTCOUNT_GET >= 0), KernelBase);

    if (ssp)
        Permit();

    core_PTPoolDispose(pool);

    /* Non-zero on success, 0 on failure - the same on every port */
    return (ret == 0);

    AROS_LIBFUNC_EXIT
}

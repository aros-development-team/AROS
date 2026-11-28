/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kjit engine: the nine operations of the interface of
    m68kemu.library for an engine (m68kemu_engine.h and ENGINE.md in
    rom/m68kemu), with their checks in the contract's order, the guest
    memory map and the counters. The translation itself is in
    m68kjit_translate.c.
*/

#include <string.h>
#include "m68kjit_intern.h"
#include "m68kjit_platform.h"

/* ---- allocation ------------------------------------------------------- */

#ifdef M68KJIT_TEST_SEAMS
long m68kjit_test_fail_alloc_at, m68kjit_test_allocs_live, m68kjit_test_exec_live;

int mj_test_fail_next(void)
{
    return m68kjit_test_fail_alloc_at > 0 && --m68kjit_test_fail_alloc_at == 0;
}
#endif

void *mj_alloc(size_t size)
{
    void *p;

#ifdef M68KJIT_TEST_SEAMS
    if (mj_test_fail_next())
        return NULL;
#endif
    p = m68kjit_plat_alloc(size);
#ifdef M68KJIT_TEST_SEAMS
    if (p)
        m68kjit_test_allocs_live++;
#endif
    return p;
}

void mj_free(void *p, size_t size)
{
    (void)size;
    if (!p)
        return;
#ifdef M68KJIT_TEST_SEAMS
    m68kjit_test_allocs_live--;
#endif
    m68kjit_plat_free(p);
}

/* ---- counters ----------------------------------------------------------- */

void mj_count(uint64_t *counter, uint64_t n)
{
    *counter = *counter > UINT64_MAX - n ? UINT64_MAX : *counter + n;
}

static ULONG mj_sat32(uint64_t v)
{
    return v > 0xFFFFFFFFu ? 0xFFFFFFFFu : (ULONG)v;
}

#ifdef M68KJIT_TEST_SEAMS
void m68kjit_test_preset_counters(struct M68KEngDomain *dom, uint64_t counts, uint64_t retired)
{
    dom->counters.blocks = dom->counters.insns = dom->counters.runs = counts;
    dom->counters.invalidations = dom->counters.code_resets = counts;
    dom->counters.retired = retired;
}
#endif

/* ---- the map ------------------------------------------------------------- */

/* The contents rule of the contract: base, size at least 0x1000, at most
   M68KENG_MAX_RANGES ranges, each non-empty, inside memory, with known flags
   and at least one, sorted and disjoint. */
static int mj_map_valid(const struct M68KEngMap *map)
{
    ULONG i;

    if (!map->base || map->size < 0x1000 || map->num_ranges > M68KENG_MAX_RANGES)
        return 0;
    for (i = 0; i < map->num_ranges; i++)
    {
        const struct M68KEngRange *r = &map->ranges[i];

        if (r->lo >= r->hi || r->hi > map->size)
            return 0;
        if (!(r->flags & (M68KENG_RANGE_READ_HOST | M68KENG_RANGE_WRITE_HOST)) ||
            (r->flags & ~(M68KENG_RANGE_READ_HOST | M68KENG_RANGE_WRITE_HOST)))
            return 0;
        if (i && r->lo < map->ranges[i - 1].hi)
            return 0;
    }
    return 1;
}

/* Takes a valid map whose base and size are the domain's: the ranges, and
   the flags of every page a range touches. */
static void mj_map_take(struct M68KEngDomain *dom)
{
    const struct M68KEngMap *map = dom->map;
    ULONG i, p;

    dom->map_generation = map->generation;
    dom->num_ranges = map->num_ranges;
    memcpy(dom->ranges, map->ranges, sizeof(dom->ranges[0]) * map->num_ranges);
    memset(dom->pages, 0, dom->npages);
    for (i = 0; i < map->num_ranges; i++)
    {
        const struct M68KEngRange *r = &map->ranges[i];
        UBYTE flag = (UBYTE)(((r->flags & M68KENG_RANGE_READ_HOST) ? M68KJIT_PAGE_READ_HOST : 0) |
                             ((r->flags & M68KENG_RANGE_WRITE_HOST) ? M68KJIT_PAGE_WRITE_HOST : 0));

        for (p = r->lo >> M68KJIT_PAGE_SHIFT; p <= (r->hi - 1) >> M68KJIT_PAGE_SHIFT; p++)
            dom->pages[p] |= flag;
    }
    dom->pages_generation++;
}

/* ---- the operations --------------------------------------------------------- */

static const struct M68KEngOps mj_ops;

static LONG mj_domain_create(const struct M68KEngHost *host, APTR host_domain,
                             struct M68KEngMap *map, ULONG required_caps,
                             struct M68KEngDomain **domain)
{
    struct M68KEngDomain *dom;
    LONG rc;

    if (domain)
        *domain = NULL;
    if (!host || !map || !domain)
        return M68KENG_ERR_ARG;
    if (host->size_of < M68KENG_V1_SIZE_HOST)
        return M68KENG_ERR_SIZE;
    if (host->abi_major != M68KENG_ABI_MAJOR)
        return M68KENG_ERR_VERSION;
    if (!host->read || !host->write)
        return M68KENG_ERR_ARG;
    if (map->size_of < M68KENG_V1_SIZE_MAP)
        return M68KENG_ERR_SIZE;
    if (!mj_map_valid(map))
        return M68KENG_ERR_ARG;
    if (required_caps & ~mj_ops.caps)
        return M68KENG_ERR_CAPS;
    if (!mj_tr_runs())
        return M68KENG_ERR_NOHOST;

    dom = mj_alloc(sizeof(*dom));
    if (!dom)
        return M68KENG_ERR_NOMEM;
    dom->host = host;
    dom->host_domain = host_domain;
    dom->map = map;
    dom->base = map->base;
    dom->size = map->size;
    dom->npages = (ULONG)(((uint64_t)map->size + (1u << M68KJIT_PAGE_SHIFT) - 1) >> M68KJIT_PAGE_SHIFT);
    dom->pages = mj_alloc(dom->npages);
    if (!dom->pages)
    {
        mj_free(dom, sizeof(*dom));
        return M68KENG_ERR_NOMEM;
    }
    mj_map_take(dom);
    rc = mj_tr_domain_create(dom);
    if (rc != M68KENG_OK)
    {
        mj_free(dom->pages, dom->npages);
        mj_free(dom, sizeof(*dom));
        return rc;
    }
    *domain = dom;
    return M68KENG_OK;
}

static LONG mj_domain_destroy(struct M68KEngDomain *dom)
{
    if (!dom)
        return M68KENG_ERR_ARG;
    if (dom->busy || dom->cpus)
        return M68KENG_ERR_STATE;
    mj_tr_domain_destroy(dom);
    mj_free(dom->pages, dom->npages);
    mj_free(dom, sizeof(*dom));
    return M68KENG_OK;
}

/* Every translation of the domain compares its guest bytes again before
   it runs, and one whose bytes changed is dropped; the range only matters
   for its size: 0 changes nothing. A range past 0xFFFFFFFF would end at
   the top of the address space; checking everything covers it. A map
   change is not this: run() drops all translations for it. */
static LONG mj_invalidate(struct M68KEngDomain *dom, ULONG addr, ULONG size)
{
    (void)addr;
    if (!dom)
        return M68KENG_ERR_ARG;
    if (dom->busy)
        return M68KENG_ERR_STATE;
    if (size == 0)
        return M68KENG_OK;
    mj_tr_invalidate(dom);
    mj_count(&dom->counters.invalidations, 1);
    return M68KENG_OK;
}

static LONG mj_cpu_create(struct M68KEngDomain *dom, APTR host_cpu, struct M68KEngCPU **cpu)
{
    struct M68KEngCPU *c;
    LONG rc;

    if (cpu)
        *cpu = NULL;
    if (!dom || !cpu)
        return M68KENG_ERR_ARG;
    if (dom->busy)
        return M68KENG_ERR_STATE;
    c = mj_alloc(sizeof(*c));
    if (!c)
        return M68KENG_ERR_NOMEM;
    c->domain = dom;
    c->host_cpu = host_cpu;
    rc = mj_tr_cpu_create(c);
    if (rc != M68KENG_OK)
    {
        mj_free(c, sizeof(*c));
        return rc;
    }
    dom->cpus++;
    *cpu = c;
    return M68KENG_OK;
}

static LONG mj_cpu_destroy(struct M68KEngCPU *c)
{
    if (!c)
        return M68KENG_ERR_ARG;
    if (c->domain->busy)
        return M68KENG_ERR_STATE;
    mj_tr_cpu_destroy(c);
    c->domain->cpus--;
    mj_free(c, sizeof(*c));
    return M68KENG_OK;
}

static LONG mj_get_reg(struct M68KEngCPU *c, ULONG reg, ULONG *value)
{
    if (!c || !value || reg >= M68KENG_REG_COUNT)
        return M68KENG_ERR_ARG;
    if (c->domain->busy)
        return M68KENG_ERR_STATE;
    *value = mj_tr_get_reg(c, reg);
    return M68KENG_OK;
}

static LONG mj_set_reg(struct M68KEngCPU *c, ULONG reg, ULONG value)
{
    if (!c || reg >= M68KENG_REG_COUNT)
        return M68KENG_ERR_ARG;
    if (c->domain->busy)
        return M68KENG_ERR_STATE;
    return mj_tr_set_reg(c, reg, value);
}

/* Checks in the contract's order; on any error nothing runs and exit is
   untouched. A changed map is checked again: base and size must stay
   those of domain_create and the contents must be valid, or the call is
   M68KENG_ERR_ARG with nothing run (an implementation rule the contract does
   not state yet). A valid new generation drops every translation of the
   domain, so the instruction fetches of this run and of every other CPU
   of the domain follow the new map; the host needs no invalidate. */
static LONG mj_run(struct M68KEngCPU *c, ULONG budget, struct M68KEngExit *exit)
{
    struct M68KEngExit out;
    struct M68KEngDomain *dom;
    const size_t from = offsetof(struct M68KEngExit, reason);

    if (!c || !exit)
        return M68KENG_ERR_ARG;
    if (exit->size_of < M68KENG_V1_SIZE_EXIT)
        return M68KENG_ERR_SIZE;
    if (budget == 0 || budget > M68KENG_MAX_BUDGET)
        return M68KENG_ERR_ARG;
    dom = c->domain;
    if (dom->busy)
        return M68KENG_ERR_STATE;
    if (dom->map->generation != dom->map_generation)
    {
        if (dom->map->base != dom->base || dom->map->size != dom->size || !mj_map_valid(dom->map))
            return M68KENG_ERR_ARG;
        mj_map_take(dom);
        mj_tr_map_changed(dom);
    }
    memset(&out, 0, sizeof(out));
    dom->busy = 1;
    mj_tr_run(c, budget, &out);
    dom->busy = 0;
    mj_count(&dom->counters.runs, 1);
    mj_count(&dom->counters.retired, out.retired);
    /* the version 1 members only: size_of and any tail stay the caller's */
    memcpy((UBYTE *)exit + from, (const UBYTE *)&out + from, M68KENG_V1_SIZE_EXIT - from);
    return M68KENG_OK;
}

static LONG mj_get_stats(struct M68KEngDomain *dom, struct M68KEngStats *stats)
{
    struct M68KEngStats s;
    const size_t from = offsetof(struct M68KEngStats, cpus);

    if (!dom || !stats)
        return M68KENG_ERR_ARG;
    if (stats->size_of < M68KENG_V1_SIZE_STATS)
        return M68KENG_ERR_SIZE;
    if (dom->busy)
        return M68KENG_ERR_STATE;
    memset(&s, 0, sizeof(s));
    s.cpus = dom->cpus;
    s.blocks_translated = mj_sat32(dom->counters.blocks);
    s.insns_translated = mj_sat32(dom->counters.insns);
    s.runs = mj_sat32(dom->counters.runs);
    s.invalidations = mj_sat32(dom->counters.invalidations);
    s.code_resets = mj_sat32(dom->counters.code_resets);
    s.code_bytes = mj_tr_code_bytes(dom);
    s.retired = dom->counters.retired;
    memcpy((UBYTE *)stats + from, (const UBYTE *)&s + from, M68KENG_V1_SIZE_STATS - from);
    return M68KENG_OK;
}

static const struct M68KEngOps mj_ops =
{
    sizeof(struct M68KEngOps), M68KENG_ABI_MAJOR, M68KENG_ABI_MINOR, M68KENG_CAP_BASE,
    M68KJIT_BLOCK_INSNS, (CONST_STRPTR)"m68kjit 1.0 (AArch64 translator)",
    mj_domain_create, mj_domain_destroy, mj_invalidate, mj_cpu_create, mj_cpu_destroy,
    mj_get_reg, mj_set_reg, mj_run, mj_get_stats
};

const struct M68KEngOps *M68KJit_GetEngineTable(ULONG abi_major)
{
    return abi_major == M68KENG_ABI_MAJOR ? &mj_ops : NULL;
}

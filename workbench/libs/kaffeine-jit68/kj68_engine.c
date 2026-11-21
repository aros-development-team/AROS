/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    kaffeine-jit68 engine: the nine operations of interface version 1
    (kaffeine-jit68-abi-v1.h; specification kaffeine-jit68-abi-v1.md), with
    their checks in the contract's order, the guest
    memory map and the counters. The translation itself is in
    kj68_translate.c.
*/

#include <stdlib.h>
#include <string.h>
#if defined(__AROS__)
#include <exec/memory.h>
#include <proto/exec.h>
#endif
#include "kj68_intern.h"

/* ---- allocation ------------------------------------------------------- */

#ifdef KJ68_TEST_SEAMS
long kj68_test_fail_alloc_at, kj68_test_allocs_live, kj68_test_exec_live;

int kj_test_fail_next(void)
{
    return kj68_test_fail_alloc_at > 0 && --kj68_test_fail_alloc_at == 0;
}
#endif

void *kj_alloc(size_t size)
{
    void *p;

#ifdef KJ68_TEST_SEAMS
    if (kj_test_fail_next())
        return NULL;
#endif
#if defined(__AROS__)
    p = AllocVec(size, MEMF_ANY | MEMF_CLEAR);
#else
    p = calloc(1, size);
#endif
#ifdef KJ68_TEST_SEAMS
    if (p)
        kj68_test_allocs_live++;
#endif
    return p;
}

void kj_free(void *p, size_t size)
{
    (void)size;
    if (!p)
        return;
#ifdef KJ68_TEST_SEAMS
    kj68_test_allocs_live--;
#endif
#if defined(__AROS__)
    FreeVec(p);
#else
    free(p);
#endif
}

/* ---- counters ----------------------------------------------------------- */

void kj_count(uint64_t *counter, uint64_t n)
{
    *counter = *counter > UINT64_MAX - n ? UINT64_MAX : *counter + n;
}

static ULONG kj_sat32(uint64_t v)
{
    return v > 0xFFFFFFFFu ? 0xFFFFFFFFu : (ULONG)v;
}

#ifdef KJ68_TEST_SEAMS
void kj68_test_preset_counters(struct KJ68Domain *dom, uint64_t counts, uint64_t retired)
{
    dom->counters.blocks = dom->counters.insns = dom->counters.runs = counts;
    dom->counters.invalidations = dom->counters.code_resets = counts;
    dom->counters.retired = retired;
}
#endif

/* ---- the map ------------------------------------------------------------- */

/* The contents rule of the contract: base, size at least 0x1000, at most
   KJ68_MAX_RANGES ranges, each non-empty, inside memory, with known flags
   and at least one, sorted and disjoint. */
static int kj_map_valid(const struct KJ68MemoryMap *map)
{
    ULONG i;

    if (!map->base || map->size < 0x1000 || map->num_ranges > KJ68_MAX_RANGES)
        return 0;
    for (i = 0; i < map->num_ranges; i++)
    {
        const struct KJ68Range *r = &map->ranges[i];

        if (r->lo >= r->hi || r->hi > map->size)
            return 0;
        if (!(r->flags & (KJ68_RANGE_READ_HOST | KJ68_RANGE_WRITE_HOST)) ||
            (r->flags & ~(KJ68_RANGE_READ_HOST | KJ68_RANGE_WRITE_HOST)))
            return 0;
        if (i && r->lo < map->ranges[i - 1].hi)
            return 0;
    }
    return 1;
}

/* Takes a valid map whose base and size are the domain's: the ranges, and
   the flags of every page a range touches. */
static void kj_map_take(struct KJ68Domain *dom)
{
    const struct KJ68MemoryMap *map = dom->map;
    ULONG i, p;

    dom->map_generation = map->generation;
    dom->num_ranges = map->num_ranges;
    memcpy(dom->ranges, map->ranges, sizeof(dom->ranges[0]) * map->num_ranges);
    memset(dom->pages, 0, dom->npages);
    for (i = 0; i < map->num_ranges; i++)
    {
        const struct KJ68Range *r = &map->ranges[i];
        UBYTE flag = (UBYTE)(((r->flags & KJ68_RANGE_READ_HOST) ? KJ68_PAGE_READ_HOST : 0) |
                             ((r->flags & KJ68_RANGE_WRITE_HOST) ? KJ68_PAGE_WRITE_HOST : 0));

        for (p = r->lo >> KJ68_PAGE_SHIFT; p <= (r->hi - 1) >> KJ68_PAGE_SHIFT; p++)
            dom->pages[p] |= flag;
    }
    dom->pages_generation++;
}

/* ---- the operations --------------------------------------------------------- */

static const struct KJ68EngineOps kj_ops;

static LONG kj_domain_create(const struct KJ68HostServices *host, APTR host_domain,
                             struct KJ68MemoryMap *map, ULONG required_caps,
                             struct KJ68Domain **domain)
{
    struct KJ68Domain *dom;
    LONG rc;

    if (domain)
        *domain = NULL;
    if (!host || !map || !domain)
        return KJ68_ERR_ARG;
    if (host->size_of < KJ68_V1_SIZE_HOST)
        return KJ68_ERR_SIZE;
    if (host->abi_major != KJ68_ABI_MAJOR)
        return KJ68_ERR_VERSION;
    if (!host->read || !host->write)
        return KJ68_ERR_ARG;
    if (map->size_of < KJ68_V1_SIZE_MAP)
        return KJ68_ERR_SIZE;
    if (!kj_map_valid(map))
        return KJ68_ERR_ARG;
    if (required_caps & ~kj_ops.caps)
        return KJ68_ERR_CAPS;
    if (!kj_tr_runs())
        return KJ68_ERR_NOHOST;

    dom = kj_alloc(sizeof(*dom));
    if (!dom)
        return KJ68_ERR_NOMEM;
    dom->host = host;
    dom->host_domain = host_domain;
    dom->map = map;
    dom->base = map->base;
    dom->size = map->size;
    dom->npages = (ULONG)(((uint64_t)map->size + (1u << KJ68_PAGE_SHIFT) - 1) >> KJ68_PAGE_SHIFT);
    dom->pages = kj_alloc(dom->npages);
    if (!dom->pages)
    {
        kj_free(dom, sizeof(*dom));
        return KJ68_ERR_NOMEM;
    }
    kj_map_take(dom);
    rc = kj_tr_domain_create(dom);
    if (rc != KJ68_OK)
    {
        kj_free(dom->pages, dom->npages);
        kj_free(dom, sizeof(*dom));
        return rc;
    }
    *domain = dom;
    return KJ68_OK;
}

static LONG kj_domain_destroy(struct KJ68Domain *dom)
{
    if (!dom)
        return KJ68_ERR_ARG;
    if (dom->busy || dom->cpus)
        return KJ68_ERR_STATE;
    kj_tr_domain_destroy(dom);
    kj_free(dom->pages, dom->npages);
    kj_free(dom, sizeof(*dom));
    return KJ68_OK;
}

/* Every translation of the domain compares its guest bytes again before
   it runs, and one whose bytes changed is dropped; the range only matters
   for its size: 0 changes nothing. A range past 0xFFFFFFFF would end at
   the top of the address space; checking everything covers it. A map
   change is not this: run() drops all translations for it. */
static LONG kj_invalidate(struct KJ68Domain *dom, ULONG addr, ULONG size)
{
    (void)addr;
    if (!dom)
        return KJ68_ERR_ARG;
    if (dom->busy)
        return KJ68_ERR_STATE;
    if (size == 0)
        return KJ68_OK;
    kj_tr_invalidate(dom);
    kj_count(&dom->counters.invalidations, 1);
    return KJ68_OK;
}

static LONG kj_cpu_create(struct KJ68Domain *dom, APTR host_cpu, struct KJ68CPU **cpu)
{
    struct KJ68CPU *c;
    LONG rc;

    if (cpu)
        *cpu = NULL;
    if (!dom || !cpu)
        return KJ68_ERR_ARG;
    if (dom->busy)
        return KJ68_ERR_STATE;
    c = kj_alloc(sizeof(*c));
    if (!c)
        return KJ68_ERR_NOMEM;
    c->domain = dom;
    c->host_cpu = host_cpu;
    rc = kj_tr_cpu_create(c);
    if (rc != KJ68_OK)
    {
        kj_free(c, sizeof(*c));
        return rc;
    }
    dom->cpus++;
    *cpu = c;
    return KJ68_OK;
}

static LONG kj_cpu_destroy(struct KJ68CPU *c)
{
    if (!c)
        return KJ68_ERR_ARG;
    if (c->domain->busy)
        return KJ68_ERR_STATE;
    kj_tr_cpu_destroy(c);
    c->domain->cpus--;
    kj_free(c, sizeof(*c));
    return KJ68_OK;
}

static LONG kj_get_reg(struct KJ68CPU *c, ULONG reg, ULONG *value)
{
    if (!c || !value || reg >= KJ68_REG_COUNT)
        return KJ68_ERR_ARG;
    if (c->domain->busy)
        return KJ68_ERR_STATE;
    *value = kj_tr_get_reg(c, reg);
    return KJ68_OK;
}

static LONG kj_set_reg(struct KJ68CPU *c, ULONG reg, ULONG value)
{
    if (!c || reg >= KJ68_REG_COUNT)
        return KJ68_ERR_ARG;
    if (c->domain->busy)
        return KJ68_ERR_STATE;
    return kj_tr_set_reg(c, reg, value);
}

/* Checks in the contract's order; on any error nothing runs and exit is
   untouched. A changed map is checked again: base and size must stay
   those of domain_create and the contents must be valid, or the call is
   KJ68_ERR_ARG with nothing run (an implementation rule the contract does
   not state yet). A valid new generation drops every translation of the
   domain, so the instruction fetches of this run and of every other CPU
   of the domain follow the new map; the host needs no invalidate. */
static LONG kj_run(struct KJ68CPU *c, ULONG budget, struct KJ68Exit *exit)
{
    struct KJ68Exit out;
    struct KJ68Domain *dom;
    const size_t from = offsetof(struct KJ68Exit, reason);

    if (!c || !exit)
        return KJ68_ERR_ARG;
    if (exit->size_of < KJ68_V1_SIZE_EXIT)
        return KJ68_ERR_SIZE;
    if (budget == 0 || budget > KJ68_MAX_BUDGET)
        return KJ68_ERR_ARG;
    dom = c->domain;
    if (dom->busy)
        return KJ68_ERR_STATE;
    if (dom->map->generation != dom->map_generation)
    {
        if (dom->map->base != dom->base || dom->map->size != dom->size || !kj_map_valid(dom->map))
            return KJ68_ERR_ARG;
        kj_map_take(dom);
        kj_tr_map_changed(dom);
    }
    memset(&out, 0, sizeof(out));
    dom->busy = 1;
    kj_tr_run(c, budget, &out);
    dom->busy = 0;
    kj_count(&dom->counters.runs, 1);
    kj_count(&dom->counters.retired, out.retired);
    /* the version 1 members only: size_of and any tail stay the caller's */
    memcpy((UBYTE *)exit + from, (const UBYTE *)&out + from, KJ68_V1_SIZE_EXIT - from);
    return KJ68_OK;
}

static LONG kj_get_stats(struct KJ68Domain *dom, struct KJ68Stats *stats)
{
    struct KJ68Stats s;
    const size_t from = offsetof(struct KJ68Stats, cpus);

    if (!dom || !stats)
        return KJ68_ERR_ARG;
    if (stats->size_of < KJ68_V1_SIZE_STATS)
        return KJ68_ERR_SIZE;
    if (dom->busy)
        return KJ68_ERR_STATE;
    memset(&s, 0, sizeof(s));
    s.cpus = dom->cpus;
    s.blocks_translated = kj_sat32(dom->counters.blocks);
    s.insns_translated = kj_sat32(dom->counters.insns);
    s.runs = kj_sat32(dom->counters.runs);
    s.invalidations = kj_sat32(dom->counters.invalidations);
    s.code_resets = kj_sat32(dom->counters.code_resets);
    s.code_bytes = kj_tr_code_bytes(dom);
    s.retired = dom->counters.retired;
    memcpy((UBYTE *)stats + from, (const UBYTE *)&s + from, KJ68_V1_SIZE_STATS - from);
    return KJ68_OK;
}

static const struct KJ68EngineOps kj_ops =
{
    sizeof(struct KJ68EngineOps), KJ68_ABI_MAJOR, KJ68_ABI_MINOR, KJ68_CAP_BASE,
    KJ68_BLOCK_INSNS, (CONST_STRPTR)"kaffeine-jit68 0.1 (J1 source, AArch64 translator)",
    kj_domain_create, kj_domain_destroy, kj_invalidate, kj_cpu_create, kj_cpu_destroy,
    kj_get_reg, kj_set_reg, kj_run, kj_get_stats
};

const struct KJ68EngineOps *KJ68_GetEngineTable(ULONG abi_major)
{
    return abi_major == KJ68_ABI_MAJOR ? &kj_ops : NULL;
}

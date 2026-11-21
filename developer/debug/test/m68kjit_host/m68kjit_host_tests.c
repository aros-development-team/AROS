/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Host checks of m68kjit through its interface. The
    engine is reached through the table M68KJit_GetEngineTable() returns, which
    the native M68KJit_GetEngine will return; the test seams of m68kjit_intern.h
    (compiled only with M68KJIT_TEST_SEAMS) inject allocation failures, count
    live allocations and executable memory, and preset counters.

    The stub host answers as the memory hooks of AROSMoira do for the
    canonical ranges (CIA and custom chip pages, low-memory guard, the
    bitplane mirror, out-of-bounds reads 0) and dispatches the few library
    calls the programs make, as m68kemu would through its exits. Programs
    and expected results of an earlier hosted suite of this translator,
    called the legacy suite below, are reused where their oracle is not
    Moira.

    Revision 2 adds map-change-fetch (a new map generation with no
    invalidate: no CPU of the domain runs code decoded under the old fetch
    rules) and zero-divisor-state (DIV.W/DIV.L through (A0)+ and -(A0):
    a zero divisor leaves every register and SR as before the division).

    Revision 3 adds div-host-read: a division whose divisor read would
    reach the host is declined before any host call (STOP_UNSUPPORTED,
    nothing done, an empty host log); direct divisors still divide.
*/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m68kemu_engine.h"
#include "m68kjit_intern.h"

#ifndef M68KJIT_TEST_SEAMS
#error "the host checks need the test seams: build with -DM68KJIT_TEST_SEAMS"
#endif

static unsigned checks, failures;
#define CHECK(x) do { checks++; if (!(x)) { failures++; \
    printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); } } while (0)

static const struct M68KEngOps *ops;

/* The layout of the legacy suite. */
#define L_ENTRY       0x1000u
#define L_HOOK_CODE   0x3000u
#define L_HOOK        0x3800u
#define L_PROBE       0x4400u
#define L_UTILITY     0x4C00u
#define L_EXEC        0x5400u
#define L_ARGS        0x5C00u
#define L_BUFFER      0x6000u
#define L_SNAPSHOT    0x6800u
#define L_SCRATCH     0x8000u
#define L_SCRATCH_END 0x10000u
#define L_DATA        0x10000u
#define L_STACK       0x40000u
#define SENTINEL      0xFFDEAD00u   /* m68kemu: the program's return address */
#define NEST_SENTINEL 0xFFDEAE00u   /* m68kemu: the return of a nested call */
#define MIB           (1u << 20)

#define R_D(n) (M68KENG_REG_D0 + (n))
#define R_A(n) (M68KENG_REG_A0 + (n))

/* ---- the stub host -------------------------------------------------------- */

struct Access
{
    ULONG addr, width, value, pc;
    int write;
};

#define MAX_LOG 64

struct Host
{
    struct M68KEngHost services;   /* first: the cookie is the host */
    struct M68KEngMap map;
    UBYTE *mem;
    ULONG size;
    int low_guard;
    struct Access log[MAX_LOG];
    unsigned nlog;
    ULONG stop_addr;
    int stop_armed;
    struct M68KEngCPU *probe_cpu;          /* engine calls tried from a callback */
    LONG probe_get_reg, probe_run;
};

static int in_custom(ULONG a) { return (a & 0xFFF000u) == 0xDFF000u; }
static int in_cia(ULONG a) { return a >= 0xBFD000u && a < 0xBFF000u; }
static int in_mem(const struct Host *h, ULONG a, ULONG w) { return (uint64_t)a + w <= h->size; }

static void log_access(struct Host *h, ULONG addr, ULONG width, ULONG value, ULONG pc, int write)
{
    if (h->nlog < MAX_LOG)
    {
        struct Access *a = &h->log[h->nlog];

        a->addr = addr;
        a->width = width;
        a->value = value;
        a->pc = pc;
        a->write = write;
    }
    h->nlog++;
}

static void probe_engine(struct Host *h)
{
    if (h->probe_cpu)
    {
        struct M68KEngExit x;
        ULONG v = 0;

        memset(&x, 0, sizeof(x));
        x.size_of = sizeof(x);
        h->probe_get_reg = ops->get_reg(h->probe_cpu, R_D(0), &v);
        h->probe_run = ops->run(h->probe_cpu, 1, &x);
    }
}

/* AROSMoira::read8/read16 for these ranges: custom registers read 0 here,
   CIA bytes 0, anything else the memory, out of bounds 0. */
static ULONG host_read(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width, ULONG pc, ULONG *value)
{
    struct Host *h = host_domain;
    ULONG v = 0;

    (void)host_cpu;
    log_access(h, addr, width, 0, pc, 0);
    probe_engine(h);
    if (width == 1)
        v = (in_cia(addr) || in_custom(addr) || !in_mem(h, addr, 1)) ? 0 : h->mem[addr];
    else if (!in_custom(addr) && in_mem(h, addr, 2))
        v = (ULONG)(h->mem[addr] << 8 | h->mem[addr + 1]);
    *value = v;
    return h->stop_armed && addr == h->stop_addr;
}

/* AROSMoira::write8/write16: custom and CIA writes do not reach memory,
   the low guard drops writes below 8 and in [0x100, 0x1C0), the bitplane
   mirror writes memory (and would mirror it). */
static ULONG host_write(APTR host_domain, APTR host_cpu, ULONG addr, ULONG width, ULONG value, ULONG pc)
{
    struct Host *h = host_domain;

    (void)host_cpu;
    log_access(h, addr, width, value, pc, 1);
    probe_engine(h);
    if (h->stop_armed && addr == h->stop_addr)
        return 1;
    if (in_custom(addr) || (width == 1 && in_cia(addr)) || !in_mem(h, addr, width))
        return 0;
    if (h->low_guard && (addr < 8 || (addr >= 0x100 && addr < 0x1C0)))
        return 0;
    if (width == 1)
        h->mem[addr] = (UBYTE)value;
    else
    {
        h->mem[addr] = (UBYTE)(value >> 8);
        h->mem[addr + 1] = (UBYTE)value;
    }
    return 0;
}

static void host_init(struct Host *h, ULONG size, UBYTE fill)
{
    memset(h, 0, sizeof(*h));
    h->mem = malloc(size);
    if (!h->mem)
    {
        printf("no memory for the guest\n");
        exit(2);
    }
    memset(h->mem, fill, size);
    h->size = size;
    h->services.size_of = sizeof(h->services);
    h->services.abi_major = M68KENG_ABI_MAJOR;
    h->services.abi_minor = M68KENG_ABI_MINOR;
    h->services.read = host_read;
    h->services.write = host_write;
    h->map.size_of = sizeof(h->map);
    h->map.generation = 1;
    h->map.base = h->mem;
    h->map.size = size;
}

static void add_range(struct Host *h, ULONG lo, ULONG hi, ULONG flags)
{
    struct M68KEngRange *r = &h->map.ranges[h->map.num_ranges++];

    r->lo = lo;
    r->hi = hi;
    r->flags = flags;
}

#define RW (M68KENG_RANGE_READ_HOST | M68KENG_RANGE_WRITE_HOST)

static void canonical_ranges(struct Host *h)
{
    add_range(h, 0, 8, M68KENG_RANGE_WRITE_HOST);
    add_range(h, 0x100, 0x1C0, M68KENG_RANGE_WRITE_HOST);
    add_range(h, 0xBFD000u, 0xBFF000u, RW);
    add_range(h, 0xDFF000u, 0xE00000u, RW);
    h->low_guard = 1;
}

/* ---- a machine: the host, one domain, its root CPU ------------------------ */

struct M
{
    struct Host h;
    struct M68KEngDomain *d;
    struct M68KEngCPU *c;
    struct M68KEngCPU *nested;    /* the CPU of nested calls, made on first use */
    ULONG pc;                  /* where the next code word goes */
    struct M68KEngExit last;      /* the exit that ended the last program */
};

static void put16(struct M *m, ULONG at, ULONG v)
{
    m->h.mem[at] = (UBYTE)(v >> 8);
    m->h.mem[at + 1] = (UBYTE)v;
}

static void put32(struct M *m, ULONG at, ULONG v)
{
    put16(m, at, v >> 16);
    put16(m, at + 2, v & 0xFFFFu);
}

static ULONG get16(const struct M *m, ULONG at)
{
    return (ULONG)(m->h.mem[at] << 8 | m->h.mem[at + 1]);
}

static ULONG get32(const struct M *m, ULONG at)
{
    return get16(m, at) << 16 | get16(m, at + 2);
}

static void w(struct M *m, ULONG v) { put16(m, m->pc, v); m->pc += 2; }
static void l(struct M *m, ULONG v) { put32(m, m->pc, v); m->pc += 4; }
static void ld(struct M *m, unsigned r, ULONG v) { w(m, 0x203Cu + (r << 9)); l(m, v); }   /* MOVE.L #v,Dr */
static void la(struct M *m, unsigned r, ULONG v) { w(m, 0x207Cu + (r << 9)); l(m, v); }   /* MOVEA.L #v,Ar */
static void rts(struct M *m) { w(m, 0x4E75); }

static void words(struct M *m, const UWORD *v, unsigned n)
{
    unsigned i;

    for (i = 0; i < n; i++)
        w(m, v[i]);
}

/* MOVEM.L D0-A7,snapshot; MOVE.W SR,snapshot+64 */
static void save(struct M *m)
{
    w(m, 0x48F9); w(m, 0xFFFF); l(m, L_SNAPSHOT);
    w(m, 0x40F9); l(m, L_SNAPSHOT + 64);
}

static void jsr_lib(struct M *m, ULONG base, ULONG lvo)
{
    la(m, 6, base);
    w(m, 0x4EAE);
    w(m, (0x10000u - lvo) & 0xFFFFu);
}

/* A0 = hook, A1 = args, A2 = buffer; CallHookPkt (utility -102) */
static void hook_call(struct M *m)
{
    la(m, 0, L_HOOK);
    la(m, 1, L_ARGS);
    la(m, 2, L_BUFFER);
    jsr_lib(m, L_UTILITY, 102);
}

static ULONG reg(struct M68KEngCPU *c, ULONG r)
{
    ULONG v = 0;

    CHECK(ops->get_reg(c, r, &v) == M68KENG_OK);
    return v;
}

static void set(struct M68KEngCPU *c, ULONG r, ULONG v)
{
    CHECK(ops->set_reg(c, r, v) == M68KENG_OK);
}

static int m_open(struct M *m, ULONG size, UBYTE fill, int canonical)
{
    ULONG lib;

    memset(m, 0, sizeof(*m));
    host_init(&m->h, size, fill);
    if (canonical)
        canonical_ranges(&m->h);
    m->pc = L_ENTRY;
    /* the jump tables of the three libraries, as the legacy suite made them */
    for (lib = L_PROBE; lib <= L_EXEC; lib += 0x800)
    {
        ULONG at;

        for (at = lib - 768; at < lib; at += 2)
            put16(m, at, 0xA000);
    }
    if (ops->domain_create(&m->h.services, &m->h, &m->h.map, 0, &m->d) != M68KENG_OK)
        return 0;
    return ops->cpu_create(m->d, m, &m->c) == M68KENG_OK;
}

static void m_close(struct M *m)
{
    if (m->nested)
        CHECK(ops->cpu_destroy(m->nested) == M68KENG_OK);
    if (m->c)
        CHECK(ops->cpu_destroy(m->c) == M68KENG_OK);
    if (m->d)
        CHECK(ops->domain_destroy(m->d) == M68KENG_OK);
    free(m->h.mem);
    memset(m, 0, sizeof(*m));
}

/* The root as m68kemu sets it before the first instruction: vectors 0 and
   4, the sentinel as the return address, A6 the "SysBase", SR 0x2700. */
static void m_start(struct M *m, ULONG entry, ULONG sysbase)
{
    ULONG r, sp = L_STACK - 4;

    put32(m, 0, L_STACK);
    put32(m, 4, sysbase);
    put32(m, sp, SENTINEL);
    for (r = 0; r < 16; r++)
        set(m->c, r, 0);
    set(m->c, R_A(7), sp);
    set(m->c, R_A(6), sysbase);
    set(m->c, M68KENG_REG_SR, 0x2700);
    set(m->c, M68KENG_REG_PC, entry);
}

/* ---- the library calls the programs make, as the host completes them ---- */

struct Seen
{
    ULONG d[8], a[8], sr;
};

static struct Seen snap;
static unsigned snap_calls;

static struct M68KEngExit run_to(struct M *m, struct M68KEngCPU *c, unsigned long max_runs);

/* The canonical Call68k: a CPU of the same domain, on a stack 256 bytes
   below the caller's, returning to NEST_SENTINEL, D0 the result. */
static ULONG call_hook(struct M *m, struct M68KEngCPU *caller)
{
    struct M68KEngExit x;
    ULONG r, hook = reg(caller, R_A(0)), sp = (reg(caller, R_A(7)) - 256) & ~3u;

    if (!m->nested)
        CHECK(ops->cpu_create(m->d, m, &m->nested) == M68KENG_OK);
    for (r = 0; r < 16; r++)
        set(m->nested, r, 0);
    set(m->nested, R_A(0), hook);
    set(m->nested, R_A(1), reg(caller, R_A(1)));
    set(m->nested, R_A(2), reg(caller, R_A(2)));
    sp -= 4;
    put32(m, sp, NEST_SENTINEL);
    set(m->nested, R_A(7), sp);
    set(m->nested, M68KENG_REG_SR, 0x2000);
    set(m->nested, M68KENG_REG_PC, get32(m, hook + 8));
    x = run_to(m, m->nested, 1000000);
    CHECK(x.reason == M68KENG_STOP_PC_OUTSIDE && x.pc == NEST_SENTINEL);
    return reg(m->nested, R_D(0));
}

/* Pops the return address, acts, sets D0, returns to the caller. 0 for a
   call these checks do not know. */
static int dispatch(struct M *m, struct M68KEngCPU *c, ULONG pc)
{
    ULONG sp = reg(c, R_A(7)), ret = get32(m, sp), d0 = reg(c, R_D(0));

    if (pc == L_PROBE - 36)            /* the probe's snapshot: D0 back */
    {
        ULONG r;

        for (r = 0; r < 8; r++)
        {
            snap.d[r] = reg(c, R_D(r));
            snap.a[r] = reg(c, R_A(r));
        }
        snap.sr = reg(c, M68KENG_REG_SR);
        snap_calls++;
    }
    else if (pc == L_EXEC - 636)       /* CacheClearU() */
        CHECK(ops->invalidate(m->d, 0, M68KENG_INVALIDATE_ALL) == M68KENG_OK);
    else if (pc == L_EXEC - 642)       /* CacheClearE(A0, D0, D1) */
        CHECK(ops->invalidate(m->d, reg(c, R_A(0)), reg(c, R_D(0))) == M68KENG_OK);
    else if (pc == L_UTILITY - 102)    /* CallHookPkt() */
        d0 = call_hook(m, c);
    else
        return 0;
    set(c, R_A(7), sp + 4);
    set(c, R_D(0), d0);
    set(c, M68KENG_REG_PC, ret);
    return 1;
}

static struct M68KEngExit run_to(struct M *m, struct M68KEngCPU *c, unsigned long max_runs)
{
    struct M68KEngExit x;
    unsigned long n;

    for (n = 0; n < max_runs; n++)
    {
        memset(&x, 0, sizeof(x));
        x.size_of = sizeof(x);
        if (ops->run(c, 4096, &x) != M68KENG_OK)
        {
            x.reason = 0;
            return x;
        }
        if (x.reason == M68KENG_STOP_BUDGET)
            continue;
        if (x.reason == M68KENG_STOP_ALINE && dispatch(m, c, x.pc))
            continue;
        return x;
    }
    memset(&x, 0, sizeof(x));
    return x;
}

/* The program's result: D0 when it returns to its sentinel, else -999 with
   the exit in m->last. */
static LONG program(struct M *m)
{
    m->last = run_to(m, m->c, 1000000);
    if (m->last.reason != M68KENG_STOP_PC_OUTSIDE || m->last.pc != SENTINEL)
        return -999;
    return (LONG)reg(m->c, R_D(0));
}

static ULONG snap_word(const struct M *m, ULONG off) { return get32(m, L_SNAPSHOT + off); }

/* A few words at L_ENTRY ended by an A-line, run once from L_ENTRY with an
   empty log; the code was written by the host, so it invalidates first. */
static struct M68KEngExit one(struct M *m, const UWORD *code, unsigned n, ULONG budget)
{
    struct M68KEngExit x;

    m->pc = L_ENTRY;
    words(m, code, n);
    w(m, 0xA000);
    CHECK(ops->invalidate(m->d, L_ENTRY, 2 * n + 2) == M68KENG_OK);
    m->h.nlog = 0;
    set(m->c, M68KENG_REG_PC, L_ENTRY);
    memset(&x, 0, sizeof(x));
    x.size_of = sizeof(x);
    CHECK(ops->run(m->c, budget, &x) == M68KENG_OK);
    return x;
}

/* the run went to the A-line that ends the words of one() */
static int ended(const struct M68KEngExit *x)
{
    return x->reason == M68KENG_STOP_ALINE;
}

/* a text starts with the words expected of it */
static int starts(const char *text, const char *with)
{
    if (!text)
        return 0;
    while (*with)
    {
        if (*text++ != *with++)
            return 0;
    }
    return 1;
}

static int logged(const struct Host *h, unsigned i, int write, ULONG addr, ULONG width)
{
    return i < h->nlog && i < MAX_LOG && h->log[i].write == write && h->log[i].addr == addr && h->log[i].width == width;
}

/* ---- 1. discovery ---------------------------------------------------------- */

static void t_discovery(void)
{
    CHECK(M68KJit_GetEngineTable(1) == ops);
    CHECK(M68KJit_GetEngineTable(0) == NULL);
    CHECK(M68KJit_GetEngineTable(2) == NULL);
    CHECK(ops->size_of >= M68KENG_V1_SIZE_OPS);
    CHECK(ops->abi_major == 1 && ops->abi_minor == 0);
    CHECK(ops->caps == M68KENG_CAP_BASE);
    CHECK(ops->max_block_insns == 64);
    CHECK(ops->max_block_insns >= 1 && ops->max_block_insns <= M68KENG_MAX_BLOCK_INSNS);
    CHECK(ops->name != NULL && ops->name[0] != 0);
    CHECK(ops->domain_create && ops->domain_destroy && ops->invalidate && ops->cpu_create && ops->cpu_destroy &&
          ops->get_reg && ops->set_reg && ops->run && ops->get_stats);
    printf("  %s, ABI %lu.%lu, %s\n", (const char *)ops->name, (unsigned long)ops->abi_major,
           (unsigned long)ops->abi_minor, mj_tr_exec_memory_kind());
}

/* ---- 2. size and version ----------------------------------------------------- */

struct HostPlus
{
    struct M68KEngHost s;
    ULONG extra[4];
};

struct MapPlus
{
    struct M68KEngMap m;
    ULONG extra[4];
};

/* domain_create with these arguments: its status, and on an error a NULL
   domain and nothing left allocated. */
static LONG try_create(const struct M68KEngHost *host, struct M68KEngMap *map, ULONG caps)
{
    struct M68KEngDomain *d = (struct M68KEngDomain *)(uintptr_t)1;
    long live = m68kjit_test_allocs_live, exec = m68kjit_test_exec_live;
    LONG rc = ops->domain_create(host, NULL, map, caps, &d);

    if (rc == M68KENG_OK)
        CHECK(d != NULL && ops->domain_destroy(d) == M68KENG_OK);
    else
        CHECK(d == NULL);
    CHECK(m68kjit_test_allocs_live == live && m68kjit_test_exec_live == exec);
    return rc;
}

static void t_sizes(void)
{
    struct Host h;
    struct M68KEngHost hs;
    struct M68KEngMap mp;
    struct HostPlus hp;
    struct MapPlus mpp;
    struct M68KEngDomain *d = (struct M68KEngDomain *)(uintptr_t)1;

    CHECK(M68KENG_V1_SIZE_EXIT == offsetof(struct M68KEngExit, text) + M68KENG_TEXT_SIZE);
    CHECK(M68KENG_V1_SIZE_STATS == offsetof(struct M68KEngStats, retired) + sizeof(UQUAD));
    CHECK(M68KENG_V1_SIZE_MAP == offsetof(struct M68KEngMap, ranges) + M68KENG_MAX_RANGES * sizeof(struct M68KEngRange));
    CHECK(M68KENG_V1_SIZE_HOST == offsetof(struct M68KEngHost, log) + sizeof(void (*)(APTR, CONST_STRPTR)));
    CHECK(M68KENG_V1_SIZE_OPS == offsetof(struct M68KEngOps, get_stats) + sizeof(void *));

    host_init(&h, MIB, 0);
    CHECK(ops->domain_create(NULL, NULL, &h.map, 0, &d) == M68KENG_ERR_ARG && d == NULL);
    CHECK(ops->domain_create(&h.services, NULL, NULL, 0, &d) == M68KENG_ERR_ARG && d == NULL);
    CHECK(ops->domain_create(&h.services, NULL, &h.map, 0, NULL) == M68KENG_ERR_ARG);

    hs = h.services; hs.size_of = M68KENG_V1_SIZE_HOST - 1;
    CHECK(try_create(&hs, &h.map, 0) == M68KENG_ERR_SIZE);
    hs = h.services; hs.abi_major = 2;
    CHECK(try_create(&hs, &h.map, 0) == M68KENG_ERR_VERSION);
    hs = h.services; hs.read = NULL;
    CHECK(try_create(&hs, &h.map, 0) == M68KENG_ERR_ARG);
    hs = h.services; hs.write = NULL;
    CHECK(try_create(&hs, &h.map, 0) == M68KENG_ERR_ARG);
    mp = h.map; mp.size_of = M68KENG_V1_SIZE_MAP - 1;
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_SIZE);
    mp = h.map; mp.base = NULL;
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    mp = h.map; mp.size = 0xFFF;
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    mp = h.map; mp.num_ranges = M68KENG_MAX_RANGES + 1;
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    mp = h.map; mp.num_ranges = 2;      /* unsorted */
    mp.ranges[0].lo = 0x2000; mp.ranges[0].hi = 0x3000; mp.ranges[0].flags = RW;
    mp.ranges[1].lo = 0x1000; mp.ranges[1].hi = 0x1800; mp.ranges[1].flags = RW;
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    mp.ranges[0].lo = 0x1000; mp.ranges[0].hi = 0x2000;      /* overlapping */
    mp.ranges[1].lo = 0x1800; mp.ranges[1].hi = 0x3000;
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    mp.num_ranges = 1;
    mp.ranges[0].lo = 0x1000; mp.ranges[0].hi = 0x1000;      /* empty */
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    mp.ranges[0].hi = MIB + 2;                               /* past the size */
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    mp.ranges[0].hi = 0x2000; mp.ranges[0].flags = 0;        /* no flag */
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    mp.ranges[0].flags = 4;                                  /* unknown flag */
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_ARG);
    CHECK(try_create(&h.services, &h.map, M68KENG_CAP_IRQ) == M68KENG_ERR_CAPS);

    /* the first failing check of the contract wins */
    hs = h.services; hs.size_of = M68KENG_V1_SIZE_HOST - 1; hs.abi_major = 2;
    CHECK(try_create(&hs, &h.map, 0) == M68KENG_ERR_SIZE);
    hs = h.services; hs.abi_major = 2; hs.read = NULL;
    CHECK(try_create(&hs, &h.map, 0) == M68KENG_ERR_VERSION);
    mp = h.map; mp.size_of = 4; mp.base = NULL;
    CHECK(try_create(&h.services, &mp, 0) == M68KENG_ERR_SIZE);
    mp = h.map; mp.base = NULL;
    CHECK(try_create(&h.services, &mp, M68KENG_CAP_IRQ) == M68KENG_ERR_ARG);

    /* exactly the v1 prefix, and an appended tail */
    hs = h.services; hs.size_of = M68KENG_V1_SIZE_HOST;
    mp = h.map; mp.size_of = M68KENG_V1_SIZE_MAP;
    CHECK(try_create(&hs, &mp, 0) == M68KENG_OK);
    memset(&hp, 0x5A, sizeof(hp));
    hp.s = h.services; hp.s.size_of = sizeof(hp);
    memset(&mpp, 0x5A, sizeof(mpp));
    mpp.m = h.map; mpp.m.size_of = sizeof(mpp);
    CHECK(try_create(&hp.s, &mpp.m, 0) == M68KENG_OK);
    CHECK(try_create(&h.services, &h.map, M68KENG_CAP_BASE) == M68KENG_OK);
    free(h.mem);
}

/* ---- 3. allocation failure ---------------------------------------------------- */

static void t_alloc_failure(void)
{
    struct Host h;
    struct M68KEngDomain *d = NULL;
    struct M68KEngCPU *c = NULL;
    long n, found = 0;

    host_init(&h, MIB, 0);
    for (n = 1; n < 32 && !found; n++)
    {
        long live = m68kjit_test_allocs_live, exec = m68kjit_test_exec_live;
        LONG rc;

        m68kjit_test_fail_alloc_at = n;
        rc = ops->domain_create(&h.services, &h, &h.map, 0, &d);
        if (rc == M68KENG_OK)
            found = n;
        else
            CHECK(rc == M68KENG_ERR_NOMEM && d == NULL && m68kjit_test_allocs_live == live && m68kjit_test_exec_live == exec);
    }
    m68kjit_test_fail_alloc_at = 0;
    printf("  domain_create: %ld allocations\n", found - 1);
    CHECK(found == 9 && d != NULL);
    found = 0;
    for (n = 1; n < 32 && !found && d; n++)
    {
        long live = m68kjit_test_allocs_live;
        LONG rc;

        m68kjit_test_fail_alloc_at = n;
        rc = ops->cpu_create(d, NULL, &c);
        if (rc == M68KENG_OK)
            found = n;
        else
            CHECK(rc == M68KENG_ERR_NOMEM && c == NULL && m68kjit_test_allocs_live == live);
    }
    m68kjit_test_fail_alloc_at = 0;
    printf("  cpu_create: %ld allocations\n", found - 1);
    CHECK(found == 4 && c != NULL);
    if (c)
        CHECK(ops->cpu_destroy(c) == M68KENG_OK);
    if (d)
        CHECK(ops->domain_destroy(d) == M68KENG_OK);
    free(h.mem);
}

/* ---- 4. lifecycle and leaks ------------------------------------------------------ */

static void t_lifecycle(void)
{
    struct Host h;
    struct M68KEngStats s;
    int round;

    host_init(&h, 2 * MIB, 0);
    CHECK(ops->domain_destroy(NULL) == M68KENG_ERR_ARG);
    CHECK(ops->cpu_destroy(NULL) == M68KENG_ERR_ARG);
    CHECK(ops->get_stats(NULL, &s) == M68KENG_ERR_ARG);
    for (round = 0; round < 3; round++)
    {
        struct M68KEngDomain *d = NULL;
        struct M68KEngCPU *c[4];
        int i;

        CHECK(ops->domain_create(&h.services, &h, &h.map, 0, &d) == M68KENG_OK);
        if (!d)
            break;
        for (i = 0; i < 4; i++)
            CHECK(ops->cpu_create(d, NULL, &c[i]) == M68KENG_OK);
        memset(&s, 0, sizeof(s));
        s.size_of = sizeof(s);
        /* a domain starts with 1 MiB of code memory */
        CHECK(ops->get_stats(d, &s) == M68KENG_OK && s.cpus == 4 && s.code_bytes == (1u << 20));
        CHECK(m68kjit_test_exec_live == (1 << 20));
        CHECK(ops->domain_destroy(d) == M68KENG_ERR_STATE);
        CHECK(ops->get_stats(d, &s) == M68KENG_OK && s.cpus == 4);
        for (i = 0; i < 4; i++)
            CHECK(ops->cpu_destroy(c[i]) == M68KENG_OK);
        CHECK(ops->get_stats(d, &s) == M68KENG_OK && s.cpus == 0);
        CHECK(ops->domain_destroy(d) == M68KENG_OK);
        CHECK(m68kjit_test_allocs_live == 0 && m68kjit_test_exec_live == 0);
    }
    free(h.mem);
}

/* ---- 5. registers ---------------------------------------------------------------- */

static void t_registers(void)
{
    struct M m;
    ULONG r, v;

    CHECK(m_open(&m, MIB, 0, 0));
    for (r = 0; r < 16; r++)
        CHECK(reg(m.c, r) == 0);
    CHECK(reg(m.c, M68KENG_REG_PC) == 0 && reg(m.c, M68KENG_REG_SR) == 0x2700);
    for (r = 0; r < 16; r++)
        set(m.c, r, 0x11110000u + r);
    set(m.c, M68KENG_REG_PC, 0x2468);
    for (r = 0; r < 16; r++)
        CHECK(reg(m.c, r) == 0x11110000u + r);
    CHECK(reg(m.c, M68KENG_REG_PC) == 0x2468);
    set(m.c, M68KENG_REG_SR, 0x2715);
    CHECK(reg(m.c, M68KENG_REG_SR) == 0x2715);
    CHECK(ops->set_reg(m.c, M68KENG_REG_SR, 0x0015) == M68KENG_ERR_UNSUPPORTED);   /* S clear */
    CHECK(ops->set_reg(m.c, M68KENG_REG_SR, 0xA715) == M68KENG_ERR_UNSUPPORTED);   /* T1 */
    CHECK(ops->set_reg(m.c, M68KENG_REG_SR, 0x6715) == M68KENG_ERR_UNSUPPORTED);   /* T0 */
    CHECK(ops->set_reg(m.c, M68KENG_REG_SR, 0x3715) == M68KENG_ERR_UNSUPPORTED);   /* M */
    CHECK(reg(m.c, M68KENG_REG_SR) == 0x2715);
    set(m.c, M68KENG_REG_SR, 0x2FFF);                                           /* reserved bits dropped */
    CHECK(reg(m.c, M68KENG_REG_SR) == 0x271F);
    CHECK(ops->get_reg(m.c, M68KENG_REG_COUNT, &v) == M68KENG_ERR_ARG);
    CHECK(ops->set_reg(m.c, M68KENG_REG_COUNT, 0) == M68KENG_ERR_ARG);
    CHECK(ops->get_reg(m.c, 0, NULL) == M68KENG_ERR_ARG);
    CHECK(ops->get_reg(NULL, 0, &v) == M68KENG_ERR_ARG);
    m_close(&m);
}

/* ---- 6. A-line ---------------------------------------------------------------------- */

static void t_aline(void)
{
    static const UWORD code[] = { 0x7005, 0x7207, 0xA123, 0x7401, 0xA124 };
    struct M m;
    struct M68KEngExit x;

    CHECK(m_open(&m, MIB, 0, 0));
    words(&m, code, 5);
    set(m.c, R_A(7), L_STACK);
    set(m.c, M68KENG_REG_PC, L_ENTRY);
    memset(&x, 0, sizeof(x));
    x.size_of = sizeof(x);
    CHECK(ops->run(m.c, 100, &x) == M68KENG_OK);
    CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == L_ENTRY + 4 && x.opcode == 0xA123 && x.retired == 2);
    CHECK(reg(m.c, R_D(0)) == 5 && reg(m.c, R_D(1)) == 7 && reg(m.c, M68KENG_REG_PC) == L_ENTRY + 4);
    set(m.c, M68KENG_REG_PC, L_ENTRY + 6);
    CHECK(ops->run(m.c, 100, &x) == M68KENG_OK);
    CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == L_ENTRY + 8 && x.opcode == 0xA124 && x.retired == 1);
    CHECK(reg(m.c, R_D(2)) == 1);
    m_close(&m);
}

/* ---- 7. sentinels and fetch ----------------------------------------------------------- */

static void t_fetch(void)
{
    static const UWORD jmp_custom[] = { 0x4EF9, 0x00DF, 0xF000 };
    struct M m;
    struct M68KEngExit x;
    ULONG s;

    CHECK(m_open(&m, MIB, 0, 0));
    for (s = 0; s < 2; s++)
    {
        ULONG sentinel = s ? NEST_SENTINEL : SENTINEL;

        put16(&m, L_ENTRY, 0x4E75);                      /* RTS */
        put32(&m, L_STACK - 4, sentinel);
        CHECK(ops->invalidate(m.d, L_ENTRY, 2) == M68KENG_OK);
        set(m.c, R_A(7), L_STACK - 4);
        set(m.c, M68KENG_REG_PC, L_ENTRY);
        memset(&x, 0, sizeof(x));
        x.size_of = sizeof(x);
        CHECK(ops->run(m.c, 100, &x) == M68KENG_OK);
        CHECK(x.reason == M68KENG_STOP_PC_OUTSIDE && x.pc == sentinel && x.address == sentinel && x.retired == 1);
    }
    set(m.c, M68KENG_REG_PC, L_ENTRY + 1);
    CHECK(ops->run(m.c, 100, &x) == M68KENG_OK);
    CHECK(x.reason == M68KENG_STOP_EXCEPTION && x.vector == 3 && x.pc == L_ENTRY + 1 && x.retired == 0);
    CHECK(reg(m.c, M68KENG_REG_PC) == L_ENTRY + 1);
    /* MOVE.L #imm,D0 whose second immediate word is past the end */
    put16(&m, MIB - 4, 0x203C);
    put16(&m, MIB - 2, 0x1234);
    set(m.c, M68KENG_REG_PC, MIB - 4);
    CHECK(ops->run(m.c, 100, &x) == M68KENG_OK);
    CHECK(x.reason == M68KENG_STOP_PC_OUTSIDE && x.pc == MIB - 4 && x.address == MIB && x.retired == 0);
    m_close(&m);

    CHECK(m_open(&m, 32 * MIB, 0, 1));
    set(m.c, M68KENG_REG_PC, 0xBFD000u);                     /* a READ_HOST range */
    memset(&x, 0, sizeof(x));
    x.size_of = sizeof(x);
    CHECK(ops->run(m.c, 100, &x) == M68KENG_OK);
    CHECK(x.reason == M68KENG_STOP_PC_OUTSIDE && x.pc == 0xBFD000u && x.address == 0xBFD000u);
    x = one(&m, jmp_custom, 3, 100);
    CHECK(x.reason == M68KENG_STOP_PC_OUTSIDE && x.pc == 0xDFF000u && x.retired == 1);
    CHECK(m.h.nlog == 0);                                 /* a fetch never goes to the host */
    m_close(&m);
}

/* One run of c from pc with budget 64. */
static struct M68KEngExit run_at(struct M68KEngCPU *c, ULONG pc)
{
    struct M68KEngExit x;

    set(c, M68KENG_REG_PC, pc);
    memset(&x, 0, sizeof(x));
    x.size_of = sizeof(x);
    CHECK(ops->run(c, 64, &x) == M68KENG_OK);
    return x;
}

static void mark_regs(struct M68KEngCPU *c)
{
    ULONG r;

    for (r = 0; r < 4; r++)
        set(c, R_D(r), 0xD0D0D0D0u + r);
}

/* A map change with no invalidate: the bytes stay, only the READ_HOST
   ranges and the generation change. No CPU of the domain may execute code
   decoded under the old fetch rules, the one whose run takes the new map
   or the other. */
static void t_map_change(void)
{
    const ULONG one_insn = L_ENTRY, three = L_ENTRY + 0x100;
    struct M68KEngCPU *cpu[2];
    struct M m;
    struct M68KEngExit x;
    unsigned i;

    CHECK(m_open(&m, MIB, 0, 0));
    cpu[0] = m.c;
    CHECK(ops->cpu_create(m.d, NULL, &cpu[1]) == M68KENG_OK);
    m.pc = one_insn;
    ld(&m, 0, 0x11223344u);                 /* MOVE.L #$11223344,D0: words at +2 and +4 */
    w(&m, 0xA000);
    m.pc = three;
    w(&m, 0x7201); w(&m, 0x7402);           /* MOVEQ #1,D1; MOVEQ #2,D2 */
    w(&m, 0x7603); w(&m, 0xA000);           /* MOVEQ #3,D3 at +4 */
    CHECK(ops->invalidate(m.d, L_ENTRY, 0x200) == M68KENG_OK);   /* the host wrote code */

    /* both CPUs translate and run both blocks under the first map */
    for (i = 0; i < 2; i++)
    {
        mark_regs(cpu[i]);
        x = run_at(cpu[i], one_insn);
        CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == one_insn + 6 && x.retired == 1 && reg(cpu[i], R_D(0)) == 0x11223344u);
        x = run_at(cpu[i], three);
        CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == three + 6 && x.retired == 3 && reg(cpu[i], R_D(3)) == 3);
    }

    /* a READ_HOST range over the second immediate word: the second CPU
       takes the new map, the first one runs after it */
    m.h.map.num_ranges = 0;
    add_range(&m.h, one_insn + 4, one_insn + 6, M68KENG_RANGE_READ_HOST);
    m.h.map.generation++;
    m.h.nlog = 0;
    for (i = 2; i-- > 0;)
    {
        mark_regs(cpu[i]);
        x = run_at(cpu[i], one_insn);
        CHECK(x.reason == M68KENG_STOP_PC_OUTSIDE && x.pc == one_insn && x.address == one_insn + 4 && x.retired == 0);
        CHECK(reg(cpu[i], R_D(0)) == 0xD0D0D0D0u && reg(cpu[i], M68KENG_REG_PC) == one_insn);
    }

    /* and one over the third opcode of the other block */
    add_range(&m.h, three + 4, three + 6, M68KENG_RANGE_READ_HOST);
    m.h.map.generation++;
    for (i = 0; i < 2; i++)
    {
        mark_regs(cpu[i]);
        x = run_at(cpu[i], three);
        CHECK(x.reason == M68KENG_STOP_PC_OUTSIDE && x.pc == three + 4 && x.address == three + 4 && x.retired == 2);
        CHECK(reg(cpu[i], R_D(1)) == 1 && reg(cpu[i], R_D(2)) == 2 && reg(cpu[i], R_D(3)) == 0xD0D0D0D3u);
    }
    CHECK(m.h.nlog == 0);                   /* a fetch never goes to the host */

    /* the ranges removed: the fetches that failed now succeed */
    m.h.map.num_ranges = 0;
    m.h.map.generation++;
    for (i = 0; i < 2; i++)
    {
        mark_regs(cpu[i]);
        x = run_at(cpu[i], three);
        CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == three + 6 && x.retired == 3 && reg(cpu[i], R_D(3)) == 3);
        x = run_at(cpu[i], one_insn);
        CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == one_insn + 6 && x.retired == 1 && reg(cpu[i], R_D(0)) == 0x11223344u);
    }

    /* a WRITE_HOST range over the code does not touch instruction fetch */
    add_range(&m.h, L_ENTRY, L_ENTRY + 0x200, M68KENG_RANGE_WRITE_HOST);
    m.h.map.generation++;
    for (i = 0; i < 2; i++)
    {
        mark_regs(cpu[i]);
        x = run_at(cpu[i], one_insn);
        CHECK(x.reason == M68KENG_STOP_ALINE && x.retired == 1 && reg(cpu[i], R_D(0)) == 0x11223344u);
        x = run_at(cpu[i], three);
        CHECK(x.reason == M68KENG_STOP_ALINE && x.retired == 3 && reg(cpu[i], R_D(3)) == 3);
    }
    CHECK(m.h.nlog == 0);
    CHECK(ops->cpu_destroy(cpu[1]) == M68KENG_OK);
    m_close(&m);
}

/* ---- 8. memory routing -------------------------------------------------------------- */

static void t_routing(void)
{
    static const UWORD move_b[] = { 0x1080 };            /* MOVE.B D0,(A0) */
    static const UWORD move_w_load[] = { 0x3210 };       /* MOVE.W (A0),D1 */
    static const UWORD move_l[] = { 0x2082 };            /* MOVE.L D2,(A0) */
    static const UWORD move_w[] = { 0x3082 };            /* MOVE.W D2,(A0) */
    static const UWORD guard_store[] = { 0x21C2, 0x0004 };   /* MOVE.L D2,$4.W */
    static const UWORD guard_load[] = { 0x2638, 0x0004 };    /* MOVE.L $4.W,D3 */
    static const UWORD rmw[] = { 0x5250 };               /* ADDQ.W #1,(A0) */
    static const UWORD movem[] = { 0x48D0, 0x0003 };     /* MOVEM.L D0-D1,(A0) */
    static const UWORD far_load[] = { 0x2839, 0x0200, 0x0000 };   /* MOVE.L $2000000,D4 */
    static const UWORD stop_prog[] = { 0x7001, 0x3080, 0x7002 };  /* MOVEQ #1,D0; MOVE.W D0,(A0); MOVEQ #2,D0 */
    struct M m;
    struct M68KEngExit x;

    CHECK(m_open(&m, 32 * MIB, 0, 1));
    set(m.c, R_A(7), L_STACK);
    set(m.c, R_D(0), 0x5A);
    set(m.c, R_D(1), 0x77777777u);
    set(m.c, R_D(2), 0x11223344u);

    set(m.c, R_A(0), 0xBFE001u);
    x = one(&m, move_b, 1, 64);
    CHECK(x.reason == M68KENG_STOP_ALINE && m.h.nlog == 1 && logged(&m.h, 0, 1, 0xBFE001u, 1));
    CHECK(m.h.log[0].value == 0x5A && m.h.log[0].pc == L_ENTRY);

    set(m.c, R_A(0), 0xDFF006u);
    x = one(&m, move_w_load, 1, 64);
    CHECK(ended(&x) && m.h.nlog == 1 && logged(&m.h, 0, 0, 0xDFF006u, 2) && reg(m.c, R_D(1)) == 0x77770000u);

    set(m.c, R_A(0), 0xDFF000u);
    x = one(&m, move_l, 1, 64);
    CHECK(ended(&x) && m.h.nlog == 2 && logged(&m.h, 0, 1, 0xDFF000u, 2) && logged(&m.h, 1, 1, 0xDFF002u, 2));
    CHECK(m.h.log[0].value == 0x1122 && m.h.log[1].value == 0x3344);

    set(m.c, R_A(0), 0xDFEFFEu);                          /* the first word direct */
    x = one(&m, move_l, 1, 64);
    CHECK(ended(&x) && m.h.nlog == 1 && logged(&m.h, 0, 1, 0xDFF000u, 2) && get16(&m, 0xDFEFFEu) == 0x1122);

    set(m.c, R_A(0), 0x10001u);                           /* misaligned, direct */
    x = one(&m, move_l, 1, 64);
    CHECK(ended(&x) && m.h.nlog == 0 && get32(&m, 0x10001u) == 0x11223344u);

    put32(&m, 4, 0xCAFEF00Du);
    x = one(&m, guard_store, 2, 64);
    CHECK(ended(&x) && m.h.nlog == 2 && logged(&m.h, 0, 1, 4, 2) && logged(&m.h, 1, 1, 6, 2) && get32(&m, 4) == 0xCAFEF00Du);
    x = one(&m, guard_load, 2, 64);
    CHECK(ended(&x) && m.h.nlog == 0 && reg(m.c, R_D(3)) == 0xCAFEF00Du);

    set(m.c, R_A(0), 0xDFF010u);
    x = one(&m, rmw, 1, 64);
    CHECK(ended(&x) && m.h.nlog == 2 && logged(&m.h, 0, 0, 0xDFF010u, 2) && logged(&m.h, 1, 1, 0xDFF010u, 2) &&
          m.h.log[1].value == 1);

    set(m.c, R_A(0), 0xDFF020u);
    x = one(&m, movem, 2, 64);
    CHECK(ended(&x) && m.h.nlog == 4 && logged(&m.h, 0, 1, 0xDFF020u, 2) && logged(&m.h, 1, 1, 0xDFF022u, 2) &&
          logged(&m.h, 2, 1, 0xDFF024u, 2) && logged(&m.h, 3, 1, 0xDFF026u, 2));

    x = one(&m, far_load, 3, 64);
    CHECK(ended(&x) && m.h.nlog == 2 && logged(&m.h, 0, 0, 32 * MIB, 2) && logged(&m.h, 1, 0, 32 * MIB + 2, 2) &&
          reg(m.c, R_D(4)) == 0);

    /* a bitplane range the host adds, then moves */
    m.h.map.ranges[4] = m.h.map.ranges[3];
    m.h.map.ranges[3] = m.h.map.ranges[2];
    m.h.map.ranges[2].lo = 0x200000u; m.h.map.ranges[2].hi = 0x201000u; m.h.map.ranges[2].flags = M68KENG_RANGE_WRITE_HOST;
    m.h.map.num_ranges = 5;
    m.h.map.generation++;
    set(m.c, R_A(0), 0x200000u);
    x = one(&m, move_w, 1, 64);
    CHECK(ended(&x) && m.h.nlog == 1 && logged(&m.h, 0, 1, 0x200000u, 2) && get16(&m, 0x200000u) == 0x3344);
    m.h.map.ranges[2].lo = 0x300000u; m.h.map.ranges[2].hi = 0x301000u;
    m.h.map.generation++;
    set(m.c, R_D(2), 0x55556666u);
    x = one(&m, move_w, 1, 64);
    CHECK(ended(&x) && m.h.nlog == 0 && get16(&m, 0x200000u) == 0x6666);
    set(m.c, R_A(0), 0x300000u);
    x = one(&m, move_w, 1, 64);
    CHECK(m.h.nlog == 1 && logged(&m.h, 0, 1, 0x300000u, 2));

    /* a map that changes its size is refused before anything runs */
    m.h.map.size = 16 * MIB;
    m.h.map.generation++;
    memset(&x, 0, sizeof(x));
    x.size_of = sizeof(x);
    CHECK(ops->run(m.c, 64, &x) == M68KENG_ERR_ARG && x.reason == 0);
    m.h.map.size = 32 * MIB;
    m.h.map.generation++;

    /* engine calls from a callback are refused */
    m.h.probe_cpu = m.c;
    set(m.c, R_A(0), 0xDFF006u);
    x = one(&m, move_w_load, 1, 64);
    CHECK(m.h.probe_get_reg == M68KENG_ERR_STATE && m.h.probe_run == M68KENG_ERR_STATE && x.reason == M68KENG_STOP_ALINE);
    m.h.probe_cpu = NULL;

    /* the host stops the run: terminal, PC at the accessing instruction */
    m.h.stop_addr = 0xDFF040u;
    m.h.stop_armed = 1;
    set(m.c, R_A(0), 0xDFF040u);
    x = one(&m, stop_prog, 3, 64);
    CHECK(x.reason == M68KENG_STOP_HOST && x.pc == L_ENTRY + 2 && reg(m.c, R_D(0)) == 1 && x.retired <= 1);
    m.h.stop_armed = 0;
    m_close(&m);
}

/* The legacy endian program: explicit expectations; its accesses outside
   memory now go to the host, and exactly these six do. */
static void t_endian(void)
{
    struct M m;
    const ULONG end = MIB;
    const UWORD code[] = {
        0x20BC, 0x1122, 0x3344,         /* MOVE.L #$11223344,(A0) */
        0x317C, 0x5566, 0x0004,         /* MOVE.W #$5566,4(A0) */
        0x117C, 0x0077, 0x0006,         /* MOVE.B #$77,6(A0) */
        0x2228, 0x0001,                 /* MOVE.L 1(A0),D1: misaligned */
        0x3428, 0x0003,                 /* MOVE.W 3(A0),D2: misaligned */
        0x1628, 0x0005,                 /* MOVE.B 5(A0),D3 */
        0x2811,                         /* MOVE.L (A1),D4 */
        0x2A28, 0x0003,                 /* MOVE.L 3(A0),D5 */
        0x24BC, 0xDEAD, 0xBEEF,         /* MOVE.L #$DEADBEEF,(A2): last long */
        0x2C12,                         /* MOVE.L (A2),D6 */
        0x2E2B, 0xFFFE,                 /* MOVE.L -2(A3),D7: one word inside */
        0x23C4, 0xFFFF, 0xFFF0,         /* MOVE.L D4,$FFFFFFF0 */
        0x2A79, 0xFFFF, 0xFFF0,         /* MOVEA.L $FFFFFFF0,A5 */
        0x377C, 0x1234, 0xFFFF,         /* MOVE.W #$1234,-1(A3) */
        0x4EAE, 0xFFDC,                 /* JSR -36(A6) */
        0x4E75
    };
    static const UBYTE want[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };

    CHECK(m_open(&m, MIB, 0xA5, 0));
    la(&m, 0, L_SCRATCH);
    la(&m, 1, L_SCRATCH + 0x100);
    la(&m, 2, end - 4);
    la(&m, 3, end);
    words(&m, code, sizeof(code) / sizeof(code[0]));
    put32(&m, L_SCRATCH + 0x100, 0xCAFEBABEu);
    m_start(&m, L_ENTRY, L_PROBE);
    snap_calls = 0;
    CHECK(program(&m) == 0 && snap_calls == 1);
    CHECK(memcmp(m.h.mem + L_SCRATCH, want, sizeof(want)) == 0);
    CHECK(snap.d[1] == 0x22334455u && (snap.d[2] & 0xFFFF) == 0x4455 && (snap.d[3] & 0xFF) == 0x66);
    CHECK(snap.d[4] == 0xCAFEBABEu && snap.d[5] == 0x44556677u);
    CHECK(snap.d[6] == 0xDEADBEEFu && snap.d[7] == 0xBEEF0000u && snap.a[5] == 0);
    CHECK(get32(&m, end - 4) == 0xDEADBEEFu);
    CHECK(m.h.nlog == 6 && logged(&m.h, 0, 0, end, 2) && logged(&m.h, 1, 1, 0xFFFFFFF0u, 2) &&
          logged(&m.h, 2, 1, 0xFFFFFFF2u, 2) && logged(&m.h, 3, 0, 0xFFFFFFF0u, 2) &&
          logged(&m.h, 4, 0, 0xFFFFFFF2u, 2) && logged(&m.h, 5, 1, end - 1, 2));
    m_close(&m);
}

/* The legacy custom-page program, with the host as AROSMoira's hooks. */
static void t_custom_page(void)
{
    struct M m;
    static const UWORD code[] = {
        0x2639, 0x00DF, 0xEFFE,         /* MOVE.L $DFEFFE,D3 */
        0x2839, 0x00DF, 0xFFFE,         /* MOVE.L $DFFFFE,D4 */
        0x3A39, 0x00DF, 0xEFFF,         /* MOVE.W $DFEFFF,D5: one access */
        0x243C, 0x1122, 0x3344,         /* MOVE.L #$11223344,D2 */
        0x23C2, 0x00DF, 0xEFFE,         /* MOVE.L D2,$DFEFFE */
        0x23C2, 0x00DF, 0xFFFE,         /* MOVE.L D2,$DFFFFE */
        0x4EAE, 0xFFDC,                 /* JSR -36(A6) */
        0x4E75
    };

    CHECK(m_open(&m, 16 * MIB, 0xA5, 0));
    add_range(&m.h, 0xDFF000u, 0xE00000u, RW);
    m.h.map.generation++;
    words(&m, code, sizeof(code) / sizeof(code[0]));
    put32(&m, 0xDFEFFCu, 0xA1A2A3A4u);
    put32(&m, 0xDFF000u, 0xB1B2B3B4u);
    put32(&m, 0xDFFFFCu, 0xC1C2C3C4u);
    put32(&m, 0xE00000u, 0xD1D2D3D4u);
    m_start(&m, L_ENTRY, L_PROBE);
    CHECK(program(&m) == 0);
    CHECK(snap.d[3] == 0xA3A40000u && snap.d[4] == 0x0000D1D2u && (snap.d[5] & 0xFFFF) == 0xA4B1);
    CHECK(get32(&m, 0xDFEFFCu) == 0xA1A21122u && get32(&m, 0xDFF000u) == 0xB1B2B3B4u);
    CHECK(get32(&m, 0xDFFFFCu) == 0xC1C2C3C4u && get32(&m, 0xE00000u) == 0x3344D3D4u);
    CHECK(m.h.nlog == 5 && logged(&m.h, 0, 0, 0xDFF000u, 2) && logged(&m.h, 1, 0, 0xDFFFFEu, 2) &&
          logged(&m.h, 2, 0, 0xDFEFFFu, 2) && logged(&m.h, 3, 1, 0xDFF000u, 2) && logged(&m.h, 4, 1, 0xDFFFFEu, 2));
    m_close(&m);
}

/* ---- 9. budget ---------------------------------------------------------------------------- */

static void t_budget(void)
{
    static const UWORD block4[] = { 0x7001, 0x5281, 0x5282, 0x60F8 };   /* MOVEQ; ADDQ.L D1; ADDQ.L D2; BRA.S start */
    static const UWORD two[] = { 0x7001, 0x7202 };
    static const UWORD spin[] = { 0x60FE };                             /* BRA.S * */
    static const ULONG bad[] = { 0, M68KENG_MAX_BUDGET + 1, 0xFFFFFFFFu };
    struct M m;
    struct M68KEngExit x, before;
    struct M68KEngStats s0, s1;
    ULONG i;

    CHECK(m_open(&m, MIB, 0, 0));
    set(m.c, R_A(7), L_STACK);
    set(m.c, M68KENG_REG_PC, L_ENTRY);
    memset(&s0, 0, sizeof(s0));
    s0.size_of = sizeof(s0);
    s1 = s0;
    CHECK(ops->get_stats(m.d, &s0) == M68KENG_OK);
    for (i = 0; i < 3; i++)
    {
        memset(&x, 0x5C, sizeof(x));
        x.size_of = sizeof(x);
        before = x;
        CHECK(ops->run(m.c, bad[i], &x) == M68KENG_ERR_ARG);
        CHECK(memcmp(&x, &before, sizeof(x)) == 0 && reg(m.c, M68KENG_REG_PC) == L_ENTRY);
    }
    CHECK(ops->get_stats(m.d, &s1) == M68KENG_OK && s1.runs == s0.runs && s1.retired == s0.retired);

    m.pc = L_ENTRY;
    words(&m, block4, 4);
    {
        const ULONG budgets[] = { 1, 3, 4, 5 }, want[] = { 4, 4, 4, 8 };

        for (i = 0; i < 4; i++)
        {
            set(m.c, M68KENG_REG_PC, L_ENTRY);
            set(m.c, R_D(1), 0);
            memset(&x, 0, sizeof(x));
            x.size_of = sizeof(x);
            CHECK(ops->run(m.c, budgets[i], &x) == M68KENG_OK);
            CHECK(x.reason == M68KENG_STOP_BUDGET && x.retired == want[i] && x.pc == L_ENTRY);
            CHECK(x.retired <= budgets[i] + ops->max_block_insns - 1);
            CHECK(reg(m.c, R_D(1)) == want[i] / 4);
        }
    }
    x = one(&m, two, 2, M68KENG_MAX_BUDGET);
    CHECK(x.reason == M68KENG_STOP_ALINE && x.retired == 2);
    m.pc = L_ENTRY;
    words(&m, spin, 1);
    CHECK(ops->invalidate(m.d, L_ENTRY, 2) == M68KENG_OK);
    set(m.c, M68KENG_REG_PC, L_ENTRY);
    memset(&x, 0, sizeof(x));
    x.size_of = sizeof(x);
    CHECK(ops->run(m.c, 1000, &x) == M68KENG_OK);
    CHECK(x.reason == M68KENG_STOP_BUDGET && x.retired == 1000 && x.pc == L_ENTRY);
    m_close(&m);
}

/* The legacy block limits: a block cut at 64 instructions whose X is read
   in the next, and a loop of 200000 back edges. */
static void t_block_limits(void)
{
    struct M m;
    struct M68KEngStats s;
    int i;
    static const UWORD loop[] = { 0x5280, 0x5381, 0x6AFA, 0x4E75 };   /* ADDQ.L #1,D0; SUBQ.L #1,D1; BPL.S; RTS */

    CHECK(m_open(&m, MIB, 0xA5, 0));
    ld(&m, 1, 0xFFFFFFFFu);
    w(&m, 0x7401);                          /* MOVEQ #1,D2 */
    for (i = 0; i < 61; i++)
        w(&m, 0x4E71);
    w(&m, 0xD481);                          /* ADD.L D1,D2: Z, C and X */
    w(&m, 0x40C3);                          /* MOVE SR,D3 */
    w(&m, 0x7800);                          /* MOVEQ #0,D4: X stays */
    w(&m, 0x40C5);                          /* MOVE SR,D5 */
    jsr_lib(&m, L_PROBE, 36);
    rts(&m);
    m_start(&m, L_ENTRY, L_PROBE);
    /* jsr_lib loads A6 itself; the legacy program had A6 = probe already */
    CHECK(program(&m) == 0);
    CHECK((snap.d[3] & 0x1F) == 0x15 && (snap.d[5] & 0x1F) == 0x14);
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK);
    printf("  64-instruction block: %lu instructions translated in %lu blocks\n",
           (unsigned long)s.insns_translated, (unsigned long)s.blocks_translated);
    CHECK(s.insns_translated == 64 + 5 + 1 + 1);
    m_close(&m);

    CHECK(m_open(&m, MIB, 0xA5, 0));
    ld(&m, 1, 199999);
    words(&m, loop, 4);
    m_start(&m, L_ENTRY, L_PROBE);
    CHECK(program(&m) == 200000);
    m_close(&m);
}

/* ---- 10. output buffers and counters -------------------------------------------------------- */

struct ExitPlus
{
    struct M68KEngExit x;
    ULONG tail[8];
};

struct StatsPlus
{
    struct M68KEngStats s;
    ULONG tail[8];
};

static int all_bytes(const void *p, size_t n, UBYTE v)
{
    const UBYTE *b = p;
    size_t i;

    for (i = 0; i < n; i++)
        if (b[i] != v)
            return 0;
    return 1;
}

static void t_outputs(void)
{
    static const UWORD two[] = { 0x7001, 0x7202 };
    struct M m;
    struct M68KEngExit x, copy;
    struct ExitPlus xp;
    struct M68KEngStats s, scopy;
    struct StatsPlus sp;
    int i;

    CHECK(m_open(&m, MIB, 0, 0));
    set(m.c, R_A(7), L_STACK);
    set(m.c, M68KENG_REG_PC, L_ENTRY);
    memset(&x, 0x3C, sizeof(x));
    x.size_of = M68KENG_V1_SIZE_EXIT - 1;
    copy = x;
    CHECK(ops->run(m.c, 10, &x) == M68KENG_ERR_SIZE && memcmp(&x, &copy, sizeof(x)) == 0);
    CHECK(reg(m.c, M68KENG_REG_PC) == L_ENTRY);
    CHECK(ops->run(NULL, 10, &x) == M68KENG_ERR_ARG);
    CHECK(ops->run(m.c, 10, NULL) == M68KENG_ERR_ARG);
    CHECK(ops->run(m.c, 0, &x) == M68KENG_ERR_SIZE);         /* size before budget */

    m.pc = L_ENTRY;
    words(&m, two, 2);
    w(&m, 0xA000);
    CHECK(ops->invalidate(m.d, L_ENTRY, 6) == M68KENG_OK);
    memset(&xp, 0xA5, sizeof(xp));
    xp.x.size_of = sizeof(xp);
    CHECK(ops->run(m.c, 10, &xp.x) == M68KENG_OK);
    CHECK(xp.x.size_of == sizeof(xp) && xp.x.reason == M68KENG_STOP_ALINE && xp.x.retired == 2);
    CHECK(all_bytes(xp.tail, sizeof(xp.tail), 0xA5));
    CHECK(all_bytes((const UBYTE *)&xp.x + M68KENG_V1_SIZE_EXIT, sizeof(xp.x) - M68KENG_V1_SIZE_EXIT, 0xA5));

    memset(&s, 0x3C, sizeof(s));
    s.size_of = M68KENG_V1_SIZE_STATS - 1;
    scopy = s;
    CHECK(ops->get_stats(m.d, &s) == M68KENG_ERR_SIZE && memcmp(&s, &scopy, sizeof(s)) == 0);
    CHECK(ops->get_stats(m.d, NULL) == M68KENG_ERR_ARG);
    memset(&sp, 0xA5, sizeof(sp));
    sp.s.size_of = sizeof(sp);
    CHECK(ops->get_stats(m.d, &sp.s) == M68KENG_OK && sp.s.size_of == sizeof(sp) && sp.s.runs == 1 && sp.s.retired == 2);
    CHECK(all_bytes(sp.tail, sizeof(sp.tail), 0xA5));

    /* saturation, by a preset only the host checks compile in */
    m68kjit_test_preset_counters(m.d, 0xFFFFFFFEu, UINT64_MAX - 2);
    for (i = 0; i < 2; i++)
    {
        set(m.c, M68KENG_REG_PC, L_ENTRY);
        memset(&x, 0, sizeof(x));
        x.size_of = sizeof(x);
        CHECK(ops->run(m.c, 10, &x) == M68KENG_OK && x.retired == 2);
    }
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK);
    CHECK(s.runs == 0xFFFFFFFFu && s.retired == UINT64_MAX);
    CHECK(s.blocks_translated == 0xFFFFFFFEu && s.insns_translated == 0xFFFFFFFEu);   /* nothing new translated */
    CHECK(s.invalidations == 0xFFFFFFFEu && s.code_resets == 0xFFFFFFFEu);
    CHECK(ops->invalidate(m.d, 0, M68KENG_INVALIDATE_ALL) == M68KENG_OK);
    CHECK(ops->invalidate(m.d, 0, M68KENG_INVALIDATE_ALL) == M68KENG_OK);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.invalidations == 0xFFFFFFFFu);
    m_close(&m);
}

/* ---- 11. invalidation ------------------------------------------------------------------------- */

static void t_invalidation(void)
{
    struct M m;
    struct M68KEngCPU *c2 = NULL;
    struct M68KEngExit x;
    struct M68KEngStats s;
    ULONG d0, before;

    CHECK(m_open(&m, MIB, 0, 0));
    CHECK(ops->cpu_create(m.d, NULL, &c2) == M68KENG_OK);
    put16(&m, L_ENTRY, 0x7001);                 /* MOVEQ #1,D0 */
    put16(&m, L_ENTRY + 2, 0xA000);
    memset(&x, 0, sizeof(x));
    x.size_of = sizeof(x);
    set(m.c, M68KENG_REG_PC, L_ENTRY);
    CHECK(ops->run(m.c, 10, &x) == M68KENG_OK && reg(m.c, R_D(0)) == 1);
    put16(&m, L_ENTRY, 0x7002);                 /* MOVEQ #2,D0, not invalidated yet */
    set(c2, M68KENG_REG_PC, L_ENTRY);
    CHECK(ops->run(c2, 10, &x) == M68KENG_OK);
    d0 = reg(c2, R_D(0));
    printf("  without invalidate the other CPU ran %s code\n", d0 == 1 ? "the old" : "the new");
    CHECK(d0 == 1 || d0 == 2);
    CHECK(ops->invalidate(m.d, L_ENTRY, 2) == M68KENG_OK);
    set(m.c, M68KENG_REG_PC, L_ENTRY);
    CHECK(ops->run(m.c, 10, &x) == M68KENG_OK && reg(m.c, R_D(0)) == 2);
    set(c2, M68KENG_REG_PC, L_ENTRY);
    CHECK(ops->run(c2, 10, &x) == M68KENG_OK && reg(c2, R_D(0)) == 2);

    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK);
    before = s.invalidations;
    CHECK(ops->invalidate(m.d, L_ENTRY, 0) == M68KENG_OK);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.invalidations == before);
    CHECK(ops->invalidate(m.d, 0xFFFFFFF0u, 0x100) == M68KENG_OK);
    CHECK(ops->invalidate(m.d, 0, M68KENG_INVALIDATE_ALL) == M68KENG_OK);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.invalidations == before + 2);
    CHECK(ops->invalidate(NULL, 0, 4) == M68KENG_ERR_ARG);
    CHECK(ops->cpu_destroy(c2) == M68KENG_OK);
    m_close(&m);
}

/* The legacy code changes: a store without a cache flush may keep the old
   translation; CacheClearU and CacheClearE make the new code count. */
static void t_code_changes(void)
{
    struct M m;
    struct M68KEngStats s;
    const ULONG sub = L_HOOK_CODE, same = sub + 0x10, results = L_BUFFER;
    static const ULONG want[] = { 1, 1, 2, 2, 3 };
    unsigned slot;

    CHECK(m_open(&m, MIB, 0xA5, 0));
    put16(&m, sub, 0x7001); put16(&m, sub + 2, 0x4E75);      /* MOVEQ #1,D0; RTS */
    put16(&m, same, 0x7205); put16(&m, same + 2, 0x4E75);    /* MOVEQ #5,D1; RTS */
    for (slot = 0; slot < 5; slot++)
    {
        w(&m, 0x4EB9); l(&m, sub);                          /* JSR sub */
        w(&m, 0x23C0); l(&m, results + 4 * slot);           /* MOVE.L D0,results+n */
        w(&m, 0x4EB9); l(&m, same);                         /* JSR same */
        if (slot == 0 || slot == 2)
        {
            w(&m, 0x33FC); w(&m, slot == 0 ? 0x7002 : 0x7003); l(&m, sub);   /* MOVE.W #,sub */
        }
        if (slot == 1)
            jsr_lib(&m, L_EXEC, 636);                       /* CacheClearU() */
        if (slot == 3)
        {
            la(&m, 0, sub); ld(&m, 0, 2); ld(&m, 1, 0x0808);
            jsr_lib(&m, L_EXEC, 642);                       /* CacheClearE(sub, 2, I|D) */
        }
    }
    rts(&m);
    m_start(&m, L_ENTRY, L_EXEC);
    CHECK(program(&m) != -999);
    for (slot = 0; slot < 5; slot++)
        CHECK(get32(&m, results + 4 * slot) == want[slot]);
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.invalidations == 2);
    m_close(&m);
}

/* The legacy code reset, now through a nested call on a second CPU: more
   blocks than the code memory keeps, and a cache flush inside the call
   while the caller waits between runs. */
static void t_code_reset(void)
{
    struct M m;
    struct M68KEngStats s;
    const unsigned units = 40000;
    unsigned i;

    CHECK(m_open(&m, MIB, 0xA5, 0));
    m.pc = 0x50000;
    for (i = 0; i < units; i++)
    {
        w(&m, 0x5280);                      /* ADDQ.L #1,D0 */
        w(&m, 0x6002); w(&m, 0x4E71);       /* BRA.S +2; NOP */
    }
    w(&m, 0x2E00);                          /* MOVE.L D0,D7 */
    jsr_lib(&m, L_EXEC, 636);               /* CacheClearU() */
    w(&m, 0x2007);                          /* MOVE.L D7,D0 */
    rts(&m);
    put32(&m, L_HOOK + 8, 0x50000);         /* h_Entry */
    m.pc = L_ENTRY;
    w(&m, 0x7000);                          /* MOVEQ #0,D0 */
    hook_call(&m);
    w(&m, 0x5A80);                          /* ADDQ.L #5,D0 */
    rts(&m);
    m_start(&m, L_ENTRY, L_EXEC);
    CHECK(program(&m) == (LONG)units + 5);
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.code_resets >= 1 && s.invalidations == 1 && s.cpus == 2);
    printf("  %u chained blocks in a nested call: %lu blocks, %lu code resets\n", units,
           (unsigned long)s.blocks_translated, (unsigned long)s.code_resets);
    m_close(&m);
}

/* The code memory, the block table and the guest-byte store double
   together when one of them is full, up to 16 MiB of code: here the block
   table fills first, four times before it has its largest size and once
   more at that size, where nothing grows any more. A failed allocation of
   the next size leaves the sizes as they are, and the program still gives
   its result. fail_at: which allocation of the first growth fails (the
   code memory, the block table, the guest bytes), 0 for none. */
static void growth_run(long fail_at)
{
    struct M m;
    struct M68KEngStats s;
    const unsigned units = 70000;
    unsigned i;
    long live;

    CHECK(m_open(&m, MIB, 0xA5, 0));
    m.pc = 0x50000;
    w(&m, 0x7000);                          /* MOVEQ #0,D0 */
    for (i = 0; i < units; i++)
    {
        w(&m, 0x5280);                      /* ADDQ.L #1,D0 */
        w(&m, 0x6002); w(&m, 0x4E71);       /* BRA.S +2; NOP */
    }
    rts(&m);
    m_start(&m, 0x50000, L_EXEC);
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.code_bytes == (1u << 20) && s.code_resets == 0);
    live = m68kjit_test_allocs_live;
    m68kjit_test_fail_alloc_at = fail_at;
    CHECK(program(&m) == (LONG)units);
    CHECK(m68kjit_test_fail_alloc_at == 0);                 /* the allocation that was to fail was made */
    m68kjit_test_fail_alloc_at = 0;
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.code_bytes == (16u << 20));
    /* a code reset at each of the four growths and one at the largest size, and one more where the first try failed */
    CHECK(s.code_resets == (fail_at ? 6u : 5u));
    CHECK(m68kjit_test_exec_live == (16 << 20) && m68kjit_test_allocs_live == live);
    printf("  %u blocks, first growth %s: %lu bytes of code memory, %lu code resets\n", units,
           fail_at ? "failed" : "made", (unsigned long)s.code_bytes, (unsigned long)s.code_resets);
    m_close(&m);
    CHECK(m68kjit_test_allocs_live == 0 && m68kjit_test_exec_live == 0);
}

/* A program that changes the same code again and again fills the block
   table with blocks nothing finds any more: the table is emptied, twice
   in 5000 turns, and nothing grows. */
static void growth_retranslate(void)
{
    struct M m;
    struct M68KEngStats s;
    unsigned i, bad = 0;

    CHECK(m_open(&m, MIB, 0xA5, 0));
    m_start(&m, L_ENTRY, L_EXEC);
    for (i = 0; i < 5000; i++)
    {
        m.pc = L_ENTRY;
        w(&m, 0x7000 + (i & 0x7F));          /* MOVEQ #i,D0 */
        rts(&m);
        /* the stack and the PC as m_start() left them: the RTS only reads the sentinel */
        if (ops->invalidate(m.d, L_ENTRY, 4) != M68KENG_OK || ops->set_reg(m.c, R_A(7), L_STACK - 4) != M68KENG_OK ||
            ops->set_reg(m.c, M68KENG_REG_PC, L_ENTRY) != M68KENG_OK)
            bad++;
        if (program(&m) != (LONG)(i & 0x7F))
            bad++;
    }
    CHECK(bad == 0);
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.code_bytes == (1u << 20) && s.code_resets == 2 &&
          s.invalidations == 5000 && s.blocks_translated == 5000);
    printf("  5000 translations of the same code: %lu bytes of code memory, %lu code resets\n",
           (unsigned long)s.code_bytes, (unsigned long)s.code_resets);
    m_close(&m);
}

static void t_growth(void)
{
    long n;

    for (n = 0; n <= 3; n++)
        growth_run(n);
    growth_retranslate();
}

/* ---- 12. exceptions and unsupported ------------------------------------------------------------- */

struct ExceptionCase
{
    UWORD code[3];
    unsigned n;
    ULONG reason, vector;
    const char *text;
};

static void t_exceptions(void)
{
    static const struct ExceptionCase cases[] = {
        { { 0x4AFC }, 1, M68KENG_STOP_EXCEPTION, 4, "ILLEGAL" },
        { { 0xF200, 0xB000 }, 2, M68KENG_STOP_EXCEPTION, 11, "Line-F" },          /* an FPU opcode */
        { { 0x4E43 }, 1, M68KENG_STOP_EXCEPTION, 35, "TRAP" },                     /* TRAP #3 */
        { { 0x80C1 }, 1, M68KENG_STOP_EXCEPTION, 5, "divide by zero" },            /* DIVU.W D1,D0, D1 = 0 */
        { { 0x4181 }, 1, M68KENG_STOP_UNSUPPORTED, 0, "opcode outside the subset" },   /* CHK.W D1,D0 */
        { { 0x46C0 }, 1, M68KENG_STOP_UNSUPPORTED, 0, "full SR write" },           /* MOVE D0,SR */
        { { 0x4E73 }, 1, M68KENG_STOP_UNSUPPORTED, 0, "opcode outside the subset" },   /* RTE */
        { { 0x4E72, 0x2000 }, 2, M68KENG_STOP_UNSUPPORTED, 0, "opcode outside the subset" },   /* STOP */
        { { 0x4E7A, 0x0801 }, 2, M68KENG_STOP_UNSUPPORTED, 0, "opcode outside the subset" },   /* MOVEC VBR,D0 */
        { { 0xF4D8 }, 1, M68KENG_STOP_UNSUPPORTED, 0, "Line-F" },                  /* CINVA */
        { { 0xF620, 0x8000 }, 2, M68KENG_STOP_UNSUPPORTED, 0, "Line-F" }          /* MOVE16 (A0)+,(A0)+ */
    };
    struct M m;
    unsigned i;

    CHECK(m_open(&m, MIB, 0xA5, 0));
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        const struct ExceptionCase *e = &cases[i];
        UWORD code[4];
        struct M68KEngExit x;

        code[0] = 0x7005;                   /* MOVEQ #5,D0 */
        memcpy(code + 1, e->code, e->n * sizeof(UWORD));
        set(m.c, R_D(1), 0);
        set(m.c, R_A(0), L_SCRATCH);
        set(m.c, R_A(7), L_STACK);
        x = one(&m, code, e->n + 1, 64);
        CHECK(x.reason == e->reason && x.pc == L_ENTRY + 2 && x.opcode == e->code[0] && x.retired == 1);
        if (e->reason == M68KENG_STOP_EXCEPTION)
            CHECK(x.vector == e->vector);
        CHECK(starts(x.text, e->text));
        CHECK(reg(m.c, R_D(0)) == 5 && reg(m.c, M68KENG_REG_PC) == L_ENTRY + 2 && reg(m.c, R_A(0)) == L_SCRATCH);
        CHECK(reg(m.c, M68KENG_REG_SR) == 0x2700);
        if (x.reason != e->reason)
            printf("  case %u: reason %lu, text \"%s\"\n", i, (unsigned long)x.reason, x.text);
    }
    m_close(&m);
}

/* DIV.W and DIV.L through (A0)+ and -(A0), after one retired instruction.
   A zero divisor in direct memory is STOP_EXCEPTION vector 5 at the
   division with nothing of it done: A0, the destination, every other
   register and SR as before it. A divisor of 7 still executes. */
struct ZeroDivCase
{
    UWORD op, ext;              /* ext 0: a word division (no extension) */
    int pre, quad;
    ULONG q, r;                 /* 100 / 7: the destination words, or Dq and Dr */
};

static void t_zero_divisor(void)
{
    static const struct ZeroDivCase cases[] = {
        { 0x80D8, 0, 0, 0, 0x0002000Eu, 0 },        /* DIVU.W (A0)+,D0 */
        { 0x80E0, 0, 1, 0, 0x0002000Eu, 0 },        /* DIVU.W -(A0),D0 */
        { 0x81D8, 0, 0, 0, 0x0002000Eu, 0 },        /* DIVS.W (A0)+,D0 */
        { 0x81E0, 0, 1, 0, 0x0002000Eu, 0 },        /* DIVS.W -(A0),D0 */
        { 0x4C58, 0x1001, 0, 0, 14, 0 },            /* DIVU.L (A0)+,D1 */
        { 0x4C60, 0x1001, 1, 0, 14, 0 },            /* DIVU.L -(A0),D1 */
        { 0x4C58, 0x1801, 0, 0, 14, 0 },            /* DIVS.L (A0)+,D1 */
        { 0x4C60, 0x1801, 1, 0, 14, 0 },            /* DIVS.L -(A0),D1 */
        { 0x4C58, 0x1402, 0, 1, 14, 2 },            /* DIVU.L (A0)+,D2:D1 */
        { 0x4C60, 0x1C02, 1, 1, 14, 2 }             /* DIVS.L -(A0),D2:D1 */
    };
    struct M m;
    unsigned i, zero;

    CHECK(m_open(&m, MIB, 0xA5, 0));
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
        for (zero = 0; zero < 2; zero++)
        {
            const struct ZeroDivCase *c = &cases[i];
            const ULONG size = c->ext ? 4 : 2, a0 = L_SCRATCH + (c->pre ? size : 0);
            const unsigned dst = c->ext ? 1 : 0;
            UWORD code[3];
            struct M68KEngExit x;
            ULONG r;

            code[0] = 0x7EFD;                   /* MOVEQ #-3,D7 */
            code[1] = c->op;
            code[2] = c->ext;
            put32(&m, L_SCRATCH, 0);
            if (!zero)
            {
                if (size == 2)
                    put16(&m, L_SCRATCH, 7);
                else
                    put32(&m, L_SCRATCH, 7);
            }
            for (r = 0; r < 7; r++)
                set(m.c, R_D(r), 0xD0000000u + r);
            for (r = 1; r < 7; r++)
                set(m.c, R_A(r), 0xA0000000u + r);
            set(m.c, R_D(dst), 100);
            if (c->quad)
                set(m.c, R_D(2), 0);
            set(m.c, R_A(0), a0);
            set(m.c, R_A(7), L_STACK);
            set(m.c, M68KENG_REG_SR, 0x2710);      /* X set */
            x = one(&m, code, c->ext ? 3 : 2, 64);
            CHECK(reg(m.c, R_D(7)) == 0xFFFFFFFDu && m.h.nlog == 0);
            for (r = 1; r < 7; r++)
                CHECK(reg(m.c, R_A(r)) == 0xA0000000u + r);
            CHECK(reg(m.c, R_A(7)) == L_STACK);
            if (zero)
            {
                CHECK(x.reason == M68KENG_STOP_EXCEPTION && x.vector == 5 && x.pc == L_ENTRY + 2 && x.opcode == c->op &&
                      x.retired == 1 && x.address == 0 && strcmp(x.text, "divide by zero") == 0);
                CHECK(reg(m.c, M68KENG_REG_PC) == L_ENTRY + 2 && reg(m.c, M68KENG_REG_SR) == 0x2718);
                CHECK(reg(m.c, R_A(0)) == a0 && reg(m.c, R_D(dst)) == 100);
                for (r = 0; r < 7; r++)
                    if (r != dst && !(c->quad && r == 2))
                        CHECK(reg(m.c, R_D(r)) == 0xD0000000u + r);
                if (c->quad)
                    CHECK(reg(m.c, R_D(2)) == 0);
                CHECK(get32(&m, L_SCRATCH) == 0);
            }
            else
            {
                CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == L_ENTRY + 2 + 2 * (c->ext ? 2 : 1) && x.retired == 2);
                CHECK(reg(m.c, R_A(0)) == (c->pre ? L_SCRATCH : L_SCRATCH + size));
                CHECK(reg(m.c, R_D(dst)) == c->q && reg(m.c, M68KENG_REG_SR) == 0x2710);
                if (c->quad)
                    CHECK(reg(m.c, R_D(2)) == c->r);
            }
            if (x.reason != (zero ? M68KENG_STOP_EXCEPTION : M68KENG_STOP_ALINE))
                printf("  case %u, divisor %s: reason %lu, text \"%s\"\n", i, zero ? "0" : "7", (unsigned long)x.reason, x.text);
        }
    m_close(&m);
}

/* The legacy faults, each after MOVEQ #5,D0, with their new outcomes. */
static void t_faults(void)
{
    static const struct ExceptionCase cases[] = {
        { { 0x4AFC }, 1, M68KENG_STOP_EXCEPTION, 4, "ILLEGAL" },
        { { 0x80C8 }, 1, M68KENG_STOP_UNSUPPORTED, 0, "DIV.W mode" },
        { { 0x4C00, 0x8000 }, 2, M68KENG_STOP_UNSUPPORTED, 0, "MUL.L extension" },
        { { 0x4C40, 0x8000 }, 2, M68KENG_STOP_UNSUPPORTED, 0, "DIV.L extension" },
        { { 0xE9C0, 0x8000 }, 2, M68KENG_STOP_UNSUPPORTED, 0, "bitfield extension" },
        { { 0x0AD0, 0x8000 }, 2, M68KENG_STOP_UNSUPPORTED, 0, "CAS extension" },
        { { 0xF200, 0xB000 }, 2, M68KENG_STOP_EXCEPTION, 11, "Line-F" },
        { { 0x4E73 }, 1, M68KENG_STOP_UNSUPPORTED, 0, "opcode outside the subset" }
    };
    static const UWORD divl_zero[] = { 0x7005, 0x4C7C, 0x0000, 0x0000, 0x0000 };   /* DIVU.L #0,D0 */
    struct M m;
    struct M68KEngExit x;
    struct M68KEngStats s;
    unsigned i;
    ULONG ext;

    CHECK(m_open(&m, MIB, 0xA5, 0));
    set(m.c, R_A(7), L_STACK);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        UWORD code[3];

        code[0] = 0x7005;
        memcpy(code + 1, cases[i].code, cases[i].n * sizeof(UWORD));
        x = one(&m, code, cases[i].n + 1, 64);
        CHECK(x.reason == cases[i].reason && x.pc == L_ENTRY + 2 && x.retired == 1);
        CHECK(starts(x.text, cases[i].text));
        if (cases[i].reason == M68KENG_STOP_EXCEPTION)
            CHECK(x.vector == cases[i].vector);
    }
    x = one(&m, divl_zero, 5, 64);
    CHECK(x.reason == M68KENG_STOP_EXCEPTION && x.vector == 5 && x.pc == L_ENTRY + 2 && x.retired == 1);
    m_close(&m);

    /* The extension words of an unsupported instruction count when a
       flush checks its block: with a brief extension word instead of a
       full one, the same MOVE.L runs on the next start. */
    CHECK(m_open(&m, MIB, 0xA5, 0));
    put32(&m, L_SCRATCH, 0x12345678u);
    jsr_lib(&m, L_EXEC, 636);                 /* CacheClearU() */
    la(&m, 0, L_SCRATCH);
    w(&m, 0x7000);                            /* MOVEQ #0,D0 */
    ext = m.pc + 2;
    w(&m, 0x2030); w(&m, 0x0100);             /* MOVE.L (0,A0,D0.W),D0, full format */
    rts(&m);
    m_start(&m, L_ENTRY, L_EXEC);
    CHECK(program(&m) == -999 && m.last.reason == M68KENG_STOP_UNSUPPORTED && m.last.pc == ext - 2);
    CHECK(strncmp(m.last.text, "MOVE source", 11) == 0);
    put16(&m, ext, 0x0000);                   /* the brief format */
    m_start(&m, L_ENTRY, L_EXEC);
    CHECK(program(&m) == 0x12345678);
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.invalidations == 2);
    m_close(&m);
}

/* ---- 13. instruction semantics -------------------------------------------------------------------- */

static void t_encoder(void)
{
    char message[160];

    CHECK(mj_tr_encoder_self_test(message, sizeof(message)) == 0);
    printf("  %s\n", message);
}

static int cmp_uword(const void *a, const void *b)
{
    UWORD x = *(const UWORD *)a, y = *(const UWORD *)b;

    return x < y ? -1 : x > y;
}

/* The legacy checksum (independent C oracle) and bubble sort (qsort). */
static void t_program(void)
{
    static const UWORD loop[] = { 0x2418, 0xEB98, 0xB580, 0xD081, 0x51C9, 0xFFF6, 0x4E75 };
    static const UWORD cmpswap[] = { 0x48E7, 0xC000, 0x3018, 0x3210, 0xB041, 0x6306, 0x3080, 0x3141, 0xFFFE,
                                     0x4CDF, 0x0003, 0x4E75 };
    struct M m;
    struct M68KEngStats s;
    ULONG i, d0 = 0, d1 = 1023, outer, inner, bsr_at;
    UWORD values[64];

    CHECK(m_open(&m, MIB, 0xA5, 0));
    w(&m, 0x41F9); l(&m, L_DATA);           /* LEA data,A0 */
    ld(&m, 1, 1023);
    w(&m, 0x7000);
    words(&m, loop, 7);
    for (i = 0; i < 1024; i++)
        put32(&m, L_DATA + 4 * i, i * 0x9E3779B9u ^ 0x5A5AA5A5u);
    for (i = 0; i < 1024; i++, d1--)
    {
        d0 = (d0 << 5) | (d0 >> 27);
        d0 ^= get32(&m, L_DATA + 4 * i);
        d0 += d1;
    }
    m_start(&m, L_ENTRY, L_PROBE);
    CHECK((ULONG)program(&m) == d0);
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    CHECK(ops->get_stats(m.d, &s) == M68KENG_OK && s.blocks_translated == 3);
    m_close(&m);

    CHECK(m_open(&m, MIB, 0xA5, 0));
    w(&m, 0x7E3E);                          /* MOVEQ #62,D7 */
    outer = m.pc;
    w(&m, 0x41F9); l(&m, L_DATA);           /* LEA data,A0 */
    w(&m, 0x3C07);                          /* MOVE.W D7,D6 */
    inner = m.pc;
    w(&m, 0x6100); bsr_at = m.pc; w(&m, 0); /* BSR.W cmpswap */
    w(&m, 0x51CE); w(&m, (inner - m.pc) & 0xFFFFu);   /* DBRA D6,inner */
    w(&m, 0x51CF); w(&m, (outer - m.pc) & 0xFFFFu);   /* DBRA D7,outer */
    w(&m, 0x4EAE); w(&m, 0xFFDC);           /* JSR -36(A6) */
    rts(&m);
    put16(&m, bsr_at, (m.pc - bsr_at) & 0xFFFFu);
    words(&m, cmpswap, 12);
    for (i = 0; i < 64; i++)
    {
        values[i] = (UWORD)((i * 0x9E37u) ^ 0x5A5Au);
        put16(&m, L_DATA + 2 * i, values[i]);
    }
    qsort(values, 64, sizeof(values[0]), cmp_uword);
    m_start(&m, L_ENTRY, L_PROBE);
    CHECK(program(&m) != -999);
    for (i = 0; i < 64; i++)
        CHECK(get16(&m, L_DATA + 2 * i) == values[i]);
    m_close(&m);
}

/* The legacy DBcc targets, with their explicit expectations. */
static void t_dbcc_targets(void)
{
    static const UWORD simple[] = { 0x7000, 0x7201, 0x5240, 0x51C9, 0xFFFC, 0x4E75 };
    struct M m;
    unsigned form;

    CHECK(m_open(&m, MIB, 0xA5, 0));
    words(&m, simple, 6);
    m_start(&m, L_ENTRY, L_PROBE);
    CHECK(program(&m) == 2);
    m_close(&m);
    for (form = 0; form < 3; form++)
    {
        CHECK(m_open(&m, MIB, 0xA5, 0));
        ld(&m, 0, form == 2 ? 3 : 0);
        ld(&m, 1, 0xC0DE0001u);
        ld(&m, 2, 0x87650000u);
        if (form == 0)
        {
            ULONG loop = m.pc;

            w(&m, 0x5240);                  /* ADDQ.W #1,D0 */
            w(&m, 0x44FC); w(&m, 0x001F);   /* MOVE #$1F,CCR */
            w(&m, 0x51C9);                  /* DBF D1,loop */
            w(&m, (loop - m.pc) & 0xFFFFu);
        }
        else
        {
            ULONG extension, target;

            w(&m, 0x44FC); w(&m, 0x001F);
            w(&m, form == 1 ? 0x51C9 : 0x57C9);   /* DBF / DBEQ D1,target */
            extension = m.pc;
            w(&m, 0);
            if (form == 1)
            {
                w(&m, 0x70FF);              /* fallthrough: MOVEQ #-1,D0 */
                w(&m, 0x6006);              /* BRA.S finish */
                w(&m, 0x4E71);
                target = m.pc;
                w(&m, 0x42C2);              /* target: MOVE CCR,D2 */
                w(&m, 0x7007);              /* MOVEQ #7,D0 */
            }
            else
            {
                w(&m, 0x6004);              /* fallthrough: BRA.S finish */
                target = m.pc;
                w(&m, 0x70FF);              /* wrongly taken: MOVEQ #-1,D0 */
                w(&m, 0x4E71);
            }
            put16(&m, extension, (target - extension) & 0xFFFFu);
        }
        save(&m);
        rts(&m);
        m_start(&m, L_ENTRY, L_PROBE);
        CHECK(program(&m) == (form == 0 ? 2 : form == 1 ? 7 : 3));
        CHECK(snap_word(&m, 4) == (form == 0 ? 0xC0DEFFFFu : form == 1 ? 0xC0DE0000u : 0xC0DE0001u));
        CHECK(snap_word(&m, 8) == (form == 1 ? 0x8765001Fu : 0x87650000u));
        CHECK((get16(&m, L_SNAPSHOT + 64) & 0x1F) == (form == 1 ? 0x10u : 0x1Fu));
        m_close(&m);
    }
}

/* The legacy word division against its independent ISA oracle. */
struct DivCase
{
    int sign;
    ULONG dividend;
    UWORD divisor;
    unsigned form, dst, flags;
};

static void div_case(const struct DivCase *c, unsigned *executed)
{
    struct M m;
    UWORD divisor = c->form == 5 ? (UWORD)c->dividend : c->divisor;
    int64_t numerator, denominator, quotient, remainder;
    int overflow;
    ULONG expected, data = L_SCRATCH;
    UWORD mask, expected_flags;
    unsigned source = (c->dst + 1) & 7, index = (c->dst + 2) & 7;
    unsigned ea = c->form == 0 ? source : c->form == 1 ? 0x3C : c->form == 2 ? 0x39 : c->form == 3 ? 0x18 :
                  c->form == 4 ? 0x20 : c->form == 5 ? c->dst : c->form == 6 ? 0x3A : 0x30;

    if (!divisor)
        return;
    ++*executed;
    numerator = c->sign ? (int64_t)(int32_t)c->dividend : (int64_t)c->dividend;
    denominator = c->sign ? (int64_t)(int16_t)divisor : (int64_t)divisor;
    quotient = numerator / denominator;
    remainder = numerator % denominator;
    overflow = quotient < (c->sign ? -32768 : 0) || quotient > (c->sign ? 32767 : 65535);
    expected = overflow ? c->dividend : (ULONG)(UWORD)remainder << 16 | (UWORD)quotient;
    mask = overflow ? 0x13 : 0x1F;
    expected_flags = (UWORD)((c->flags & 0x10) | (overflow ? 2 : ((quotient & 0x8000) ? 8 : 0) | (quotient == 0 ? 4 : 0)));

    CHECK(m_open(&m, MIB, 0xA5, 0));
    ld(&m, source, 0xC0DE0000u | divisor);
    ld(&m, index, 0xC0DE0004u);
    ld(&m, c->dst, c->dividend);
    la(&m, 0, c->form == 4 ? data + 2 : c->form == 7 ? data - 4 : data);
    put16(&m, data, divisor);
    w(&m, 0x44FC); w(&m, c->flags);         /* MOVE #flags,CCR */
    w(&m, (c->sign ? 0x81C0u : 0x80C0u) | (c->dst << 9) | ea);
    if (c->form == 1) w(&m, divisor);
    if (c->form == 2) l(&m, data);
    if (c->form == 6) w(&m, (data - m.pc) & 0xFFFFu);
    if (c->form == 7) w(&m, index << 12);
    save(&m);
    rts(&m);
    m_start(&m, L_ENTRY, L_PROBE);
    CHECK(program(&m) != -999);
    CHECK(snap_word(&m, 4 * c->dst) == expected);
    CHECK((get16(&m, L_SNAPSHOT + 64) & mask) == expected_flags);
    if (c->form == 3 || c->form == 4)
        CHECK(snap_word(&m, 32) == L_SCRATCH + (c->form == 3 ? 2 : 0));
    CHECK(m.h.mem[L_SCRATCH] == (UBYTE)(divisor >> 8) && m.h.mem[L_SCRATCH + 1] == (UBYTE)divisor);
    m_close(&m);
}

static void t_word_division(void)
{
    static const ULONG dividends[] = { 0, 1, 2, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF,
        0x8000, 0xFFFF, 0x10000, 0x1FFFF, 0x7FFFFFFF, 0x80000000u, 0x80000001u, 0xFFFFFFFFu };
    static const UWORD divisors[] = { 1, 2, 3, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF };
    unsigned serial = 0, executed = 0, i, j, sign;
    ULONG seed = 0xD1706802u;
    struct DivCase c;

    for (sign = 0; sign < 2; sign++)
        for (i = 0; i < 16; i++)
            for (j = 0; j < 12; j++)
            {
                unsigned k = serial++;

                c.sign = (int)sign; c.dividend = dividends[i]; c.divisor = divisors[j];
                c.form = k % 8; c.dst = (k / 8) % 8; c.flags = (k * 7) & 31;
                div_case(&c, &executed);
            }
    for (i = 0; i < 512; i++)
    {
        ULONG dividend, divisor;

        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; dividend = seed;
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; divisor = seed;
        c.sign = (int)(i & 1); c.dividend = dividend; c.divisor = (UWORD)((divisor & 0xFFFF) ? divisor : 1);
        c.form = i % 8; c.dst = (i / 8) % 8;
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; c.flags = seed & 31;
        div_case(&c, &executed);
    }
    printf("  %u word-division cases against the independent oracle\n", executed);
    for (sign = 0; sign < 2; sign++)
    {
        struct M m;

        CHECK(m_open(&m, MIB, 0xA5, 0));
        ld(&m, 0, 0x12345678u);
        ld(&m, 1, 0xC0DE0000u);
        w(&m, sign ? 0x81C1 : 0x80C1);
        w(&m, 0x23FC); l(&m, 0xBAD); l(&m, L_DATA);
        rts(&m);
        m_start(&m, L_ENTRY, L_PROBE);
        CHECK(program(&m) == -999 && m.last.reason == M68KENG_STOP_EXCEPTION && m.last.vector == 5);
        CHECK(get32(&m, L_DATA) == 0xA5A5A5A5u);
        m_close(&m);
    }
}

/* The legacy full-index checks against the independent integer address
   oracle; a read outside memory reads 0 through the host. */
static void fi_seed(struct M *m)
{
    unsigned i;

    for (i = 0; i < 8; i++) ld(m, i, 0xD0000000u + i);
    for (i = 0; i < 7; i++) la(m, i, 0xA0000000u + i);
}

static ULONG fi_word(const struct M *m, ULONG at)
{
    return (uint64_t)at + 2 <= MIB ? get16(m, at) : 0;
}

static void t_full_index(void)
{
    static const ULONG targets[] = { L_SCRATCH + 3696 + 7 * 4, MIB - 1, MIB };
    static const UWORD reserved[] = { 0x0100, 0x0118, 0x0111, 0x0112, 0x0113, 0x0114, 0x0115, 0x0116, 0x0117 };
    unsigned t, pcrel, bd_size, suppress, long_index, scale, address_index, i;
    struct M m;

    for (t = 0; t < 3; t++)
    {
        CHECK(m_open(&m, MIB, 0xA5, 0));
        fi_seed(&m);
        la(&m, 0, 7);
        la(&m, 2, targets[t] - 3696 - 7 * 4);
        w(&m, 0x44FC); w(&m, 0x001F);
        w(&m, 0x4232); w(&m, 0x8D20); w(&m, 0x0E70);    /* CLR.B (3696,A2,A0.L*4), full format */
        save(&m);
        rts(&m);
        m_start(&m, L_ENTRY, L_PROBE);
        CHECK(program(&m) != -999);
        CHECK(targets[t] >= MIB || m.h.mem[targets[t]] == 0);
        CHECK((get16(&m, L_SNAPSHOT + 64) & 0x1F) == 0x14);
        m_close(&m);
    }
    for (pcrel = 0; pcrel < 2; pcrel++)
        for (bd_size = 1; bd_size <= 3; bd_size++)
            for (suppress = 0; suppress < 4; suppress++)
                for (long_index = 0; long_index < 2; long_index++)
                    for (scale = 0; scale < 4; scale++)
                        for (address_index = 0; address_index < 2; address_index++)
                        {
                            const int bs = suppress & 1, is = (suppress & 2) != 0;
                            const ULONG index = long_index ? ((scale & 1) ? 0xFFFFFFFDu : 3u) :
                                ((scale & 1) ? 0xCAFEFFFDu : 0xCAFE0003u);
                            const LONG disp = bd_size == 1 ? 0 : bd_size == 2 ?
                                ((scale & 1) ? -3696 : 3696) : ((scale & 1) ? INT32_MIN : INT32_MAX);
                            const ULONG scaled = (long_index ? index : (ULONG)(LONG)(int16_t)index) << scale;
                            const ULONG base = bs ? 0xBAD00002u : L_SCRATCH - (ULONG)disp - (is ? 0 : scaled);
                            const ULONG ext = (address_index ? 0x8000u : 0x1000u) | (long_index ? 0x0800u : 0) |
                                (scale << 9) | 0x0100u | (bs ? 0x0080u : 0) | (is ? 0x0040u : 0) | (bd_size << 4);
                            ULONG expected_address, read_address, expected_value, pcbase;

                            CHECK(m_open(&m, MIB, 0xA5, 0));
                            fi_seed(&m);
                            la(&m, 2, base);
                            if (address_index) la(&m, 0, index); else ld(&m, 1, index);
                            w(&m, 0x44FC); w(&m, 0x001F);
                            w(&m, pcrel ? 0x49FB : 0x49F2);             /* LEA <ea>,A4 */
                            pcbase = m.pc;
                            expected_address = (bs ? 0 : pcrel ? pcbase : base) + (ULONG)disp + (is ? 0 : scaled);
                            w(&m, ext);
                            if (bd_size == 2) w(&m, (ULONG)disp & 0xFFFFu);
                            if (bd_size == 3) l(&m, (ULONG)disp);
                            w(&m, pcrel ? 0x263B : 0x2632);             /* MOVE.L <ea>,D3 */
                            pcbase = m.pc;
                            read_address = (bs ? 0 : pcrel ? pcbase : base) + (ULONG)disp + (is ? 0 : scaled);
                            w(&m, ext);
                            if (bd_size == 2) w(&m, (ULONG)disp & 0xFFFFu);
                            if (bd_size == 3) l(&m, (ULONG)disp);
                            save(&m);
                            rts(&m);
                            m_start(&m, L_ENTRY, L_PROBE);
                            /* both entry vectors are in place, as the legacy adapters left them */
                            expected_value = fi_word(&m, read_address) << 16 | fi_word(&m, read_address + 2);
                            CHECK(program(&m) != -999);
                            CHECK(snap_word(&m, 48) == expected_address);
                            CHECK(snap_word(&m, 12) == expected_value);
                            CHECK((get16(&m, L_SNAPSHOT + 64) & 0x1F) ==
                                  (0x10u | (expected_value == 0 ? 4u : (expected_value & 0x80000000u) ? 8u : 0u)));
                            m_close(&m);
                        }
    for (i = 0; i < 9; i++)
    {
        static const UWORD prefix[] = { 0x207C, 0x0000, 0x8000, 0x203C, 0x1234, 0x5678 };   /* A0, D0 */
        UWORD code[9];
        struct M68KEngExit x;

        CHECK(m_open(&m, MIB, 0xA5, 0));
        memcpy(code, prefix, sizeof(prefix));
        code[6] = 0x4232; code[7] = reserved[i]; code[8] = 0x4E75;
        set(m.c, R_A(2), L_SCRATCH);
        set(m.c, R_A(7), L_STACK);
        x = one(&m, code, 9, 64);
        CHECK(x.reason == M68KENG_STOP_UNSUPPORTED && strstr(x.text, "unary mode") != NULL);
        CHECK(all_bytes(m.h.mem + L_SCRATCH, L_SCRATCH_END - L_SCRATCH, 0xA5));
        m_close(&m);
    }
    for (i = 0; i < 2; i++)
    {
        ULONG bytes = i ? 6 : 4, entry = MIB - bytes;
        struct M68KEngExit x;

        CHECK(m_open(&m, MIB, 0xA5, 0));
        put16(&m, entry, 0x4AB2);
        put16(&m, entry + 2, 0x0130);
        if (bytes == 6)
            put16(&m, MIB - 2, 0x1234);
        set(m.c, M68KENG_REG_PC, entry);
        memset(&x, 0, sizeof(x));
        x.size_of = sizeof(x);
        CHECK(ops->run(m.c, 64, &x) == M68KENG_OK);
        CHECK(x.reason == M68KENG_STOP_PC_OUTSIDE && x.pc == entry && x.address == MIB);
        m_close(&m);
    }
}

/* ---- the run ------------------------------------------------------------------------------------- */

static void group(const char *name, void (*fn)(void))
{
    unsigned before = failures;

    printf("RUN: %s\n", name);
    fn();
    CHECK(m68kjit_test_allocs_live == 0 && m68kjit_test_exec_live == 0);   /* nothing left behind */
    printf("%s: %s\n", name, failures == before ? "PASS" : "FAIL");
}

/* ---- 26. a divisor that the host would read (revision 3) ------------------- */

/* A division whose divisor read would reach the host (a READ_HOST range,
   either word of a longword, or outside guest memory) is declined before
   any host call: STOP_UNSUPPORTED at the division, nothing of it done, the
   host log empty, whatever the divisor's value. Divisors read directly
   still divide: next to the range, on its page, across a page boundary,
   under a range that only sends writes to the host. */
#define DH_LO 0x9000u
#define DH_HI 0x9010u
#define DH_FIRST_DIRECT 16      /* the cases before it are declined */

struct DivHostCase
{
    UWORD op[3];
    unsigned n;          /* words of the division */
    ULONG a0, a1;        /* address registers before */
    ULONG target;        /* where the divisor is */
    int wide, quad;      /* DIV.L, and its 64-bit form (D2:D1) */
    int host;            /* 1: declined */
    ULONG a0_after;      /* A0 after an executed division */
};

static const struct DivHostCase div_host_cases[] = {
    /* declined: the divisor read would reach the host */
    { { 0x80D0 }, 1, DH_LO, 0xA0000001u, DH_LO, 0, 0, 1, 0 },                       /* DIVU.W (A0),D0 */
    { { 0x81D0 }, 1, DH_LO + 4, 0xA0000001u, DH_LO + 4, 0, 0, 1, 0 },               /* DIVS.W (A0),D0 */
    { { 0x80D8 }, 1, DH_LO + 2, 0xA0000001u, DH_LO + 2, 0, 0, 1, 0 },               /* DIVU.W (A0)+,D0 */
    { { 0x81E0 }, 1, DH_LO + 6, 0xA0000001u, DH_LO + 4, 0, 0, 1, 0 },               /* DIVS.W -(A0),D0 */
    { { 0x80E8, 0x0008 }, 2, DH_LO - 8, 0xA0000001u, DH_LO, 0, 0, 1, 0 },           /* DIVU.W 8(A0),D0 */
    { { 0x80F0, 0x9002 }, 2, DH_LO, 4, DH_LO + 6, 0, 0, 1, 0 },                     /* DIVU.W 2(A0,A1.W),D0 */
    { { 0x80F9, 0x0000, 0x9000 }, 3, 0, 0xA0000001u, DH_LO, 0, 0, 1, 0 },           /* DIVU.W $9000.L,D0 */
    { { 0x80FA, 0x7FFC }, 2, 0, 0xA0000001u, DH_LO, 0, 0, 1, 0 },                   /* DIVU.W $9000(PC),D0 */
    { { 0x80D0 }, 1, DH_LO - 1, 0xA0000001u, DH_LO - 1, 0, 0, 1, 0 },               /* word ending in the range */
    { { 0x80D0 }, 1, MIB - 1, 0xA0000001u, MIB - 1, 0, 0, 1, 0 },                   /* word past the end */
    { { 0x4C50, 0x1001 }, 2, DH_LO - 2, 0xA0000001u, DH_LO - 2, 1, 0, 1, 0 },       /* DIVU.L (A0),D1: second word */
    { { 0x4C58, 0x1801 }, 2, DH_LO + 12, 0xA0000001u, DH_LO + 12, 1, 0, 1, 0 },     /* DIVS.L (A0)+,D1 */
    { { 0x4C60, 0x1402 }, 2, DH_HI, 0xA0000001u, DH_HI - 4, 1, 1, 1, 0 },           /* DIVU.L -(A0),D2:D1 */
    { { 0x4C60, 0x1C02 }, 2, DH_LO + 4, 0xA0000001u, DH_LO, 1, 1, 1, 0 },           /* DIVS.L -(A0),D2:D1 */
    { { 0x4C50, 0x1001 }, 2, MIB - 2, 0xA0000001u, MIB - 2, 1, 0, 1, 0 },           /* DIVU.L, second word past the end */
    { { 0x4C50, 0x1001 }, 2, 0xFFFFFFFEu, 0xA0000001u, 0, 1, 0, 1, 0 },             /* DIVU.L at the top: wraps */
    /* executed: the divisor is read directly */
    { { 0x80D0 }, 1, DH_LO - 2, 0xA0000001u, DH_LO - 2, 0, 0, 0, DH_LO - 2 },       /* just below the range */
    { { 0x80D0 }, 1, DH_HI, 0xA0000001u, DH_HI, 0, 0, 0, DH_HI },                   /* just past it */
    { { 0x81D0 }, 1, DH_LO + 0x100, 0xA0000001u, DH_LO + 0x100, 0, 0, 0, DH_LO + 0x100 },  /* its page, outside it */
    { { 0x4C50, 0x1001 }, 2, DH_LO - 4, 0xA0000001u, DH_LO - 4, 1, 0, 0, DH_LO - 4 },      /* DIVU.L below it */
    { { 0x80D8 }, 1, 0x10FFFu, 0xA0000001u, 0x10FFFu, 0, 0, 0, 0x11001u },          /* across a page, (A0)+ */
    { { 0x4C60, 0x1C02 }, 2, 0x11002u, 0xA0000001u, 0x10FFEu, 1, 1, 0, 0x10FFEu },  /* across a page, -(A0), 64-bit */
    { { 0x80F8, 0x0004 }, 2, 0xA0000000u, 0xA0000001u, 4, 0, 0, 0, 0xA0000000u }    /* $4.W: a write-only range */
};

static struct M68KEngExit div_host_one(struct M *m, const struct DivHostCase *c, ULONG divisor)
{
    UWORD code[4];
    ULONG r;

    code[0] = 0x7EFD;                       /* MOVEQ #-3,D7 */
    memcpy(code + 1, c->op, c->n * sizeof(UWORD));
    if ((uint64_t)c->target + (c->wide ? 4u : 2u) <= MIB)   /* none past the end */
    {
        if (c->wide)
            put32(m, c->target, divisor);
        else
            put16(m, c->target, divisor);
    }
    for (r = 0; r < 7; r++)
        set(m->c, R_D(r), 0xD0000000u + r);
    for (r = 2; r < 7; r++)
        set(m->c, R_A(r), 0xA0000000u + r);
    set(m->c, R_D(c->wide ? 1 : 0), 100);
    if (c->quad)
        set(m->c, R_D(2), 0);
    set(m->c, R_A(0), c->a0);
    set(m->c, R_A(1), c->a1);
    set(m->c, R_A(7), L_STACK);
    set(m->c, M68KENG_REG_SR, 0x2710);         /* X set */
    return one(m, code, c->n + 1, 64);
}

/* What every declined division leaves: the exit, the registers, the memory,
   the empty host log. */
static void div_host_declined(struct M *m, const struct DivHostCase *c, const struct M68KEngExit *x, ULONG divisor)
{
    unsigned dst = c->wide ? 1 : 0;
    ULONG r;

    CHECK(x->reason == M68KENG_STOP_UNSUPPORTED && x->pc == L_ENTRY + 2 && x->retired == 1 && x->opcode == c->op[0] &&
          strcmp(x->text, "divisor read through the host") == 0);
    CHECK(m->h.nlog == 0);
    CHECK(reg(m->c, M68KENG_REG_PC) == L_ENTRY + 2 && reg(m->c, M68KENG_REG_SR) == 0x2718);
    CHECK(reg(m->c, R_D(7)) == 0xFFFFFFFDu && reg(m->c, R_D(dst)) == 100);
    for (r = 0; r < 7; r++)
        if (r != dst && !(c->quad && r == 2))
            CHECK(reg(m->c, R_D(r)) == 0xD0000000u + r);
    if (c->quad)
        CHECK(reg(m->c, R_D(2)) == 0);
    CHECK(reg(m->c, R_A(0)) == c->a0 && reg(m->c, R_A(1)) == c->a1);
    for (r = 2; r < 7; r++)
        CHECK(reg(m->c, R_A(r)) == 0xA0000000u + r);
    CHECK(reg(m->c, R_A(7)) == L_STACK);
    if ((uint64_t)c->target + (c->wide ? 4u : 2u) <= MIB)
        CHECK((c->wide ? get32(m, c->target) : get16(m, c->target)) == divisor);
}

static void t_div_host_read(void)
{
    static const UWORD indirect[] = { 0x7EFD, 0x80F0, 0x0111 };   /* DIVU.W ([A0,D0.W]),D0: memory indirect */
    static const UWORD ill[] = { 0x7EFD, 0x4AFC };                /* ILLEGAL */
    static const UWORD load[] = { 0x3210 };                       /* MOVE.W (A0),D1 */
    const unsigned ncases = sizeof(div_host_cases) / sizeof(div_host_cases[0]);
    const struct DivHostCase *host = &div_host_cases[0], *direct = &div_host_cases[DH_FIRST_DIRECT];
    struct M68KEngCPU *other = NULL;
    struct M68KEngExit x;
    struct M m;
    unsigned i, value;

    CHECK(m_open(&m, MIB, 0, 0));
    add_range(&m.h, 0, 8, M68KENG_RANGE_WRITE_HOST);
    add_range(&m.h, DH_LO, DH_HI, M68KENG_RANGE_READ_HOST);
    m.h.map.generation++;

    /* each case with a zero divisor and with 7 */
    for (i = 0; i < ncases; i++)
        for (value = 0; value < 2; value++)
        {
            const struct DivHostCase *c = &div_host_cases[i];
            const ULONG divisor = value ? 7 : 0;

            x = div_host_one(&m, c, divisor);
            if (c->host)
                div_host_declined(&m, c, &x, divisor);
            else if (!divisor)
            {
                CHECK(x.reason == M68KENG_STOP_EXCEPTION && x.vector == 5 && x.pc == L_ENTRY + 2 && x.retired == 1 && m.h.nlog == 0);
                CHECK(reg(m.c, R_A(0)) == c->a0);
            }
            else
            {
                CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == L_ENTRY + 2 + 2 * c->n && x.retired == 2 && m.h.nlog == 0);
                CHECK(reg(m.c, R_A(0)) == c->a0_after && reg(m.c, M68KENG_REG_SR) == 0x2710);
                CHECK(reg(m.c, R_D(c->wide ? 1 : 0)) == (c->wide ? 14u : 0x0002000Eu));
                if (c->quad)
                    CHECK(reg(m.c, R_D(2)) == 2);
            }
            if (c->host ? x.reason != M68KENG_STOP_UNSUPPORTED : x.reason != (divisor ? M68KENG_STOP_ALINE : M68KENG_STOP_EXCEPTION))
                printf("  case %u, divisor %lu: reason %lu, text \"%s\"\n", i, (unsigned long)divisor, (unsigned long)x.reason, x.text);
        }

    /* a memory indirect address is outside the subset: no pointer is read */
    set(m.c, R_A(0), DH_LO);
    set(m.c, R_D(0), 0);
    x = one(&m, indirect, 3, 64);
    CHECK(x.reason == M68KENG_STOP_UNSUPPORTED && x.pc == L_ENTRY + 2 && strcmp(x.text, "DIV.W mode") == 0 && m.h.nlog == 0);

    /* nothing is left over: a later fault is classified as before, ordinary
       reads through the host still reach it */
    x = div_host_one(&m, host, 7);
    div_host_declined(&m, host, &x, 7);
    x = one(&m, ill, 2, 64);
    CHECK(x.reason == M68KENG_STOP_EXCEPTION && x.vector == 4 && strcmp(x.text, "ILLEGAL") == 0);
    x = div_host_one(&m, direct, 0);
    CHECK(x.reason == M68KENG_STOP_EXCEPTION && x.vector == 5 && strcmp(x.text, "divide by zero") == 0);
    put16(&m, DH_LO, 0x0123);
    set(m.c, R_A(0), DH_LO);
    set(m.c, R_D(1), 0x77777777u);
    x = one(&m, load, 1, 64);
    CHECK(x.reason == M68KENG_STOP_ALINE && m.h.nlog == 1 && logged(&m.h, 0, 0, DH_LO, 2) && reg(m.c, R_D(1)) == 0x77770123u);

    /* the probe is made at run time: the same translation, kept from the
       declined run (no invalidate), divides or declines by the address */
    x = div_host_one(&m, host, 7);
    div_host_declined(&m, host, &x, 7);
    m.h.nlog = 0;
    set(m.c, R_A(0), DH_LO + 0x100);
    put16(&m, DH_LO + 0x100, 7);
    set(m.c, R_D(0), 100);
    x = run_at(m.c, L_ENTRY);
    CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == L_ENTRY + 4 && x.retired == 2 && m.h.nlog == 0 && reg(m.c, R_D(0)) == 0x0002000Eu);
    set(m.c, R_A(0), DH_LO);
    set(m.c, R_D(0), 100);
    x = run_at(m.c, L_ENTRY);
    CHECK(x.reason == M68KENG_STOP_UNSUPPORTED && x.pc == L_ENTRY + 2 && x.retired == 1 && m.h.nlog == 0 && reg(m.c, R_D(0)) == 100);

    /* after a decline the host can execute the division itself and go on */
    set(m.c, R_D(0), 0x0002000Eu);
    x = run_at(m.c, L_ENTRY + 4);
    CHECK(x.reason == M68KENG_STOP_ALINE && x.pc == L_ENTRY + 4 && x.retired == 0 && m.h.nlog == 0);

    /* another CPU of the domain: its own decline, no state from the first */
    CHECK(ops->cpu_create(m.d, NULL, &other) == M68KENG_OK);
    if (other)
    {
        m.h.nlog = 0;
        set(other, R_A(0), DH_LO);
        set(other, R_D(0), 100);
        set(other, R_A(7), L_STACK);
        set(other, M68KENG_REG_SR, 0x2700);
        x = run_at(other, L_ENTRY + 2);
        CHECK(x.reason == M68KENG_STOP_UNSUPPORTED && x.pc == L_ENTRY + 2 && m.h.nlog == 0 && reg(other, R_D(0)) == 100);
        set(m.c, R_A(0), DH_LO + 0x100);
        set(m.c, R_D(0), 100);
        x = run_at(m.c, L_ENTRY + 2);
        CHECK(x.reason == M68KENG_STOP_ALINE && reg(m.c, R_D(0)) == 0x0002000Eu);
        set(other, R_A(0), DH_LO + 0x100);
        x = run_at(other, L_ENTRY + 2);
        CHECK(x.reason == M68KENG_STOP_ALINE && m.h.nlog == 0 && reg(other, R_D(0)) == 0x0002000Eu);
        CHECK(ops->cpu_destroy(other) == M68KENG_OK);
    }

    /* a new map generation: without the range the division reads directly,
       with it again it is declined */
    m.h.map.num_ranges = 0;
    m.h.map.generation++;
    m.h.nlog = 0;
    put16(&m, DH_LO, 7);
    set(m.c, R_A(0), DH_LO);
    set(m.c, R_D(0), 100);
    x = run_at(m.c, L_ENTRY + 2);
    CHECK(x.reason == M68KENG_STOP_ALINE && m.h.nlog == 0 && reg(m.c, R_D(0)) == 0x0002000Eu);
    add_range(&m.h, DH_LO, DH_HI, M68KENG_RANGE_READ_HOST);
    m.h.map.generation++;
    set(m.c, R_A(0), DH_LO);
    set(m.c, R_D(0), 100);
    x = run_at(m.c, L_ENTRY + 2);
    CHECK(x.reason == M68KENG_STOP_UNSUPPORTED && m.h.nlog == 0 && reg(m.c, R_D(0)) == 100);
    m_close(&m);
}

int main(void)
{
    ops = M68KJit_GetEngineTable(M68KENG_ABI_MAJOR);
    if (!ops)
    {
        printf("no engine for ABI 1\n");
        return 1;
    }
    if (!mj_tr_runs())
    {
        printf("SKIP: translated code runs only on AArch64 Darwin or Linux (%s)\n", mj_tr_exec_memory_kind());
        return 1;
    }
    group("discovery", t_discovery);                 /* 1 */
    group("size-and-version", t_sizes);              /* 2 */
    group("allocation-failure", t_alloc_failure);    /* 3 */
    group("lifecycle-and-leaks", t_lifecycle);       /* 4 */
    group("registers", t_registers);                 /* 5 */
    group("a-line", t_aline);                        /* 6 */
    group("sentinels-and-fetch", t_fetch);           /* 7 */
    group("map-change-fetch", t_map_change);         /* 7, revision 2 */
    group("memory-routing", t_routing);              /* 8 */
    group("endian", t_endian);                       /* 8, legacy */
    group("custom-page", t_custom_page);             /* 8, legacy */
    group("budget", t_budget);                       /* 9 */
    group("block-limits", t_block_limits);           /* 9, legacy */
    group("output-buffers-and-counters", t_outputs); /* 10 */
    group("invalidation", t_invalidation);           /* 11 */
    group("code-changes", t_code_changes);           /* 11, legacy */
    group("code-reset", t_code_reset);               /* 11, legacy */
    group("growth", t_growth);                       /* 11 */
    group("exceptions-and-unsupported", t_exceptions);   /* 12 */
    group("zero-divisor-state", t_zero_divisor);     /* 12, revision 2 */
    group("faults", t_faults);                       /* 12, legacy */
    group("encoder", t_encoder);                     /* 13 */
    group("program", t_program);                     /* 13, legacy */
    group("dbcc-targets", t_dbcc_targets);           /* 13, legacy */
    group("word-division", t_word_division);         /* 13, legacy */
    group("full-index", t_full_index);               /* 13, legacy */
    group("div-host-read", t_div_host_read);         /* 12, revision 3 */
    printf("m68kjit host checks (%s): %u checks, %u failed\n", mj_tr_exec_memory_kind(), checks, failures);
    return failures ? 1 : 0;
}

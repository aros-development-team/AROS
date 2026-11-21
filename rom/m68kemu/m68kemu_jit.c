/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library - the optional engine: which CPU runs the program,
    the engine's view of the program's memory, and the end of it
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "m68kemu_intern.h"

/*
 * An engine is a library with the kaffeine-jit68 interface, version 1
 * (kaffeine-jit68-abi-v1.h in workbench/libs/kaffeine-jit68, where the
 * engine is). It translates the program's 68k code and runs
 * it between the instructions the interpreter has to take: every library
 * call, every exception and every interrupt stays with Moira and the code
 * in m68kemu_moira.cpp, which hands the registers over for a slice and
 * takes them back. Without M68KEMU_CPU nothing of this runs.
 */

/* a line about the engine, where the library's other messages go */
void m68k_jit_say(struct M68KEmuContext *ctx, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    m68k_log(ctx, "%s", buf);
}

static void jit_log(APTR host_domain, CONST_STRPTR text)
{
    struct M68KEmuContext *ctx = (struct M68KEmuContext *)host_domain;

    if (ctx->trace_calls && text)
        bug("[m68kemu] engine: %s\n", (const char *)text);
}

/* The engine's map of the program's memory: the ranges whose accesses the
   host has to see. The first page (the vectors, the guard on them and the
   host's stubs), the CIA and custom chip registers, and the bitplanes a
   program draws into while a screen mirrors them. The bitplane range comes
   and goes with a thunk, between engine runs: the map follows it here.
   FALSE while that range cannot be told apart from a fixed one. */
BOOL m68k_jit_map(struct M68KEmuContext *ctx)
{
    static const struct KJ68Range fixed[3] =
    {
        { 0x000000, 0x001000, KJ68_RANGE_WRITE_HOST },
        { M68KEMU_CIA_BASE, M68KEMU_CIA_END, KJ68_RANGE_READ_HOST | KJ68_RANGE_WRITE_HOST },
        { M68KEMU_CUSTOM_BASE, M68KEMU_CUSTOM_BASE + 0x1000, KJ68_RANGE_READ_HOST | KJ68_RANGE_WRITE_HOST },
    };
    struct KJ68MemoryMap *m = &ctx->jit_map;
    ULONG lo = 0, hi = 0, n = 0, i;
    BOOL ok = TRUE, placed = FALSE;

    if (ctx->plane_base && ctx->plane_screen && ctx->plane_base < ctx->mem_size)
    {
        lo = ctx->plane_base;
        hi = lo + ctx->plane_size * ctx->plane_depth;
        if (hi > ctx->mem_size || hi < lo)
            hi = ctx->mem_size;
        if (hi == lo)
            lo = hi = 0;
    }
    if (m->size_of && lo == ctx->jit_plane_lo && hi == ctx->jit_plane_hi)
        return ctx->jit_map_ok;

    for (i = 0; i < 3; i++)
    {
        struct KJ68Range r = fixed[i];

        if (r.lo >= ctx->mem_size)
            break;
        if (r.hi > ctx->mem_size)
            r.hi = ctx->mem_size;
        if (hi > lo && lo < r.hi && r.lo < hi)
            ok = FALSE;
        if (hi > lo && !placed && ok && hi <= r.lo)
        {
            m->ranges[n].lo = lo;
            m->ranges[n].hi = hi;
            m->ranges[n++].flags = KJ68_RANGE_WRITE_HOST;
            placed = TRUE;
        }
        m->ranges[n++] = r;
    }
    if (hi > lo && !placed && ok)
    {
        m->ranges[n].lo = lo;
        m->ranges[n].hi = hi;
        m->ranges[n++].flags = KJ68_RANGE_WRITE_HOST;
    }
    m->size_of = sizeof(*m);
    m->base = ctx->mem;
    m->size = ctx->mem_size;
    m->num_ranges = n;
    m->generation++;
    ctx->jit_plane_lo = lo;
    ctx->jit_plane_hi = hi;
    ctx->jit_map_ok = ok;
    return ok;
}

/* the engine of a library, for this interface */
static const struct KJ68EngineOps *jit_engine(struct Library *base)
{
    return (const struct KJ68EngineOps *)AROS_LC1(APTR, KJ68_GetEngine,
        AROS_LCA(ULONG, KJ68_ABI_MAJOR, D0),
        struct Library *, base, 5, KJ68);
}

/* Open the engine and give it the program's memory: NULL when the program
   can run on it, otherwise why not */
static const char *jit_open(struct M68KEmuContext *ctx)
{
    const struct KJ68EngineOps *ops;
    struct KJ68Domain *domain = NULL;

    ctx->jit_base = OpenLibrary((CONST_STRPTR)KJ68_LIBNAME, KJ68_LIBVERSION);
    if (!ctx->jit_base)
        return KJ68_LIBNAME " is not there";
    ops = jit_engine(ctx->jit_base);
    if (!ops)
        return KJ68_LIBNAME " has no engine for interface 1";
    /* the minor of interface 1.0 is 0: every minor is at least that */
    if (ops->size_of < KJ68_V1_SIZE_ENGINEOPS || ops->abi_major != KJ68_ABI_MAJOR ||
        !(ops->caps & KJ68_CAP_BASE) || !ops->max_block_insns || ops->max_block_insns > KJ68_MAX_BLOCK_INSNS ||
        !ops->name || !ops->domain_create || !ops->domain_destroy || !ops->invalidate || !ops->cpu_create ||
        !ops->cpu_destroy || !ops->get_reg || !ops->set_reg || !ops->run || !ops->get_stats)
        return "the engine's table is not one of interface 1.0";

    ctx->jit_host.size_of = sizeof(ctx->jit_host);
    ctx->jit_host.abi_major = KJ68_ABI_MAJOR;
    ctx->jit_host.abi_minor = KJ68_ABI_MINOR;
    ctx->jit_host.read = M68KEmu_JitRead;
    ctx->jit_host.write = M68KEmu_JitWrite;
    ctx->jit_host.log = jit_log;
    m68k_jit_map(ctx);

    ctx->jit_status = ops->domain_create(&ctx->jit_host, ctx, &ctx->jit_map, KJ68_CAP_BASE, &domain);
    if (ctx->jit_status != KJ68_OK || !domain)
        return ctx->jit_status == KJ68_ERR_NOMEM  ? "the engine has no memory for the program" :
               ctx->jit_status == KJ68_ERR_NOHOST ? "the engine cannot run on this machine" :
                                                    "the engine refused the program's memory";
    ctx->jit_ops = ops;
    ctx->jit_domain = domain;
    return NULL;
}

/* M68KEMU_CPU, read once before the program is loaded: unset or empty is
   the interpreter, as without this file; "interpreter" says so; "jit" is
   the engine or no run at all; "auto" is the engine when it is there and
   works. -1 when the program is not to run. */
static LONG jit_select(struct M68KEmuContext *ctx)
{
    char v[32];
    const char *why;
    LONG n = GetVar((CONST_STRPTR)"M68KEMU_CPU", (STRPTR)v, sizeof(v), 0);

    if (n <= 0)
        return 0;
    v[sizeof(v) - 1] = 0;
    /* a value that fills the buffer may have been cut: none of the three is that long */
    if (n >= (LONG)sizeof(v) - 1)
    {
        m68k_jit_say(ctx, "M68KEMU_CPU starts \"%s\" and is too long: it can be interpreter, jit or auto; the program is not run", v);
        return -1;
    }
    n = (LONG)strlen(v);
    while (n > 0 && (v[n - 1] == '\n' || v[n - 1] == '\r' || v[n - 1] == ' '))
        v[--n] = 0;
    if (!n)
        return 0;

    if (!strcmp(v, "interpreter"))
    {
        ctx->jit_mode = M68KJIT_INTERPRETER;
        return 0;
    }
    if (!strcmp(v, "jit"))
        ctx->jit_mode = M68KJIT_JIT;
    else if (!strcmp(v, "auto"))
        ctx->jit_mode = M68KJIT_AUTO;
    else
    {
        m68k_jit_say(ctx, "M68KEMU_CPU is \"%s\": it can be interpreter, jit or auto; the program is not run", v);
        return -1;
    }

    why = jit_open(ctx);
    if (!why)
    {
        ctx->jit_run = TRUE;
        return 0;
    }
    m68k_jit_close(ctx);
    if (ctx->jit_mode == M68KJIT_JIT)
    {
        m68k_jit_say(ctx, "M68KEMU_CPU is jit and %s; the program is not run", why);
        return -1;
    }
    ctx->jit_why = why;
    return 0;
}

/* Without the variable the caller sees nothing of this, its IoErr() included */
LONG m68k_jit_select(struct M68KEmuContext *ctx)
{
    LONG err = IoErr(), r = jit_select(ctx);

    if (r == 0)
        SetIoErr(err);
    return r;
}

/* the program's name for a line: the end of it when it is a long path */
const char *m68k_jit_name(struct M68KEmuContext *ctx)
{
    ULONG len = strlen(ctx->program_name);

    if (!len)
        return "68k program";
    return ctx->program_name + (len > 48 ? len - 48 : 0);
}

/* The program wrote code, or the host did for it (a loader, a jump table, a
   stub, a cache call of the program): what the engine translated is void.
   Called by the task that holds the program's lock, or before any CPU. */
void m68k_jit_invalidate(struct M68KEmuContext *ctx)
{
    if (ctx->jit_domain)
        ctx->jit_ops->invalidate(ctx->jit_domain, 0, KJ68_INVALIDATE_ALL);
}

/* after a run on the engine: what it did, and what the interpreter did */
void m68k_jit_report(struct M68KEmuContext *ctx)
{
    struct KJ68Stats s;

    if (!ctx->jit_domain || !ctx->jit_run)
        return;
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    if (ctx->jit_ops->get_stats(ctx->jit_domain, &s) != KJ68_OK)
        return;
    m68k_jit_say(ctx, "%s: the engine ran %lu instructions in %lu runs (translated: %lu blocks, %lu instructions; "
                      "%lu invalidations, %lu code resets); the interpreter ran %lu",
        m68k_jit_name(ctx),
        (unsigned long)s.retired, (unsigned long)s.runs, (unsigned long)s.blocks_translated,
        (unsigned long)s.insns_translated, (unsigned long)s.invalidations, (unsigned long)s.code_resets,
        (unsigned long)ctx->jit_interp);
}

/* The end of the engine for this program: after its last CPU, before the
   program's memory is freed. The library is closed last. */
void m68k_jit_close(struct M68KEmuContext *ctx)
{
    if (ctx->jit_domain)
    {
        LONG st = ctx->jit_ops->domain_destroy(ctx->jit_domain);

        ctx->jit_domain = NULL;
        if (st != KJ68_OK)
        {
            /* a CPU of the program is still there: the engine keeps what it
               holds, and so does the library */
            m68k_jit_say(ctx, "the engine kept its domain (status %ld)", (long)st);
            ctx->jit_base = NULL;
            ctx->jit_kept = TRUE;
        }
    }
    ctx->jit_ops = NULL;
    ctx->jit_run = FALSE;
    if (ctx->jit_base)
    {
        CloseLibrary(ctx->jit_base);
        ctx->jit_base = NULL;
    }
}

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
 * An engine is a separate library with the interface of m68kemu_engine.h,
 * version 1 (specified in ENGINE.md). It runs the program's 68k code
 * between the instructions the interpreter has to take: every library
 * call, every exception and every interrupt stays with Moira and the code
 * in m68kemu_moira.cpp, which hands the registers over for a slice and
 * takes them back. Without M68KEMU_CPU nothing of this runs.
 */

/* The engine library this file looks for, and the lowest version of it that
   has the function of the interface: a translator of 68k code for AArch64
   targets, a library of its own. Where it is not there, "jit" refuses the
   run and "auto" uses the interpreter. Any library that implements the
   interface can take this place: the name is here and nowhere else. */
#define M68KEMU_ENGINE_LIBRARY  "m68kjit.library"
#define M68KEMU_ENGINE_VERSION  1

/* a line about the engine, where the library's other messages go */
void m68k_engine_say(struct M68KEmuContext *ctx, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    m68k_log(ctx, "%s", buf);
}

static void eng_log(APTR host_domain, CONST_STRPTR text)
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
BOOL m68k_engine_map(struct M68KEmuContext *ctx)
{
    static const struct M68KEngRange fixed[3] =
    {
        { 0x000000, 0x001000, M68KENG_RANGE_WRITE_HOST },
        { M68KEMU_CIA_BASE, M68KEMU_CIA_END, M68KENG_RANGE_READ_HOST | M68KENG_RANGE_WRITE_HOST },
        { M68KEMU_CUSTOM_BASE, M68KEMU_CUSTOM_BASE + 0x1000, M68KENG_RANGE_READ_HOST | M68KENG_RANGE_WRITE_HOST },
    };
    struct M68KEngMap *m = &ctx->eng_map;
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
    if (m->size_of && lo == ctx->eng_plane_lo && hi == ctx->eng_plane_hi)
        return ctx->eng_map_ok;

    for (i = 0; i < 3; i++)
    {
        struct M68KEngRange r = fixed[i];

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
            m->ranges[n++].flags = M68KENG_RANGE_WRITE_HOST;
            placed = TRUE;
        }
        m->ranges[n++] = r;
    }
    if (hi > lo && !placed && ok)
    {
        m->ranges[n].lo = lo;
        m->ranges[n].hi = hi;
        m->ranges[n++].flags = M68KENG_RANGE_WRITE_HOST;
    }
    m->size_of = sizeof(*m);
    m->base = ctx->mem;
    m->size = ctx->mem_size;
    m->num_ranges = n;
    m->generation++;
    ctx->eng_plane_lo = lo;
    ctx->eng_plane_hi = hi;
    ctx->eng_map_ok = ok;
    return ok;
}

/* the engine of a library, for this interface */
static const struct M68KEngOps *eng_table(struct Library *base)
{
    return (const struct M68KEngOps *)AROS_LC1(APTR, M68KEng_GetEngine,
        AROS_LCA(ULONG, M68KENG_ABI_MAJOR, D0),
        struct Library *, base, 5, M68KEng);
}

/* Open the engine and give it the program's memory: NULL when the program
   can run on it, otherwise why not */
static const char *eng_open(struct M68KEmuContext *ctx)
{
    const struct M68KEngOps *ops;
    struct M68KEngDomain *domain = NULL;

    ctx->eng_base = OpenLibrary((CONST_STRPTR)M68KEMU_ENGINE_LIBRARY, M68KEMU_ENGINE_VERSION);
    if (!ctx->eng_base)
        return M68KEMU_ENGINE_LIBRARY " is not there";
    ops = eng_table(ctx->eng_base);
    if (!ops)
        return M68KEMU_ENGINE_LIBRARY " has no engine for interface 1";
    /* the minor of interface 1.0 is 0: every minor is at least that */
    if (ops->size_of < M68KENG_V1_SIZE_OPS || ops->abi_major != M68KENG_ABI_MAJOR ||
        !(ops->caps & M68KENG_CAP_BASE) || !ops->max_block_insns || ops->max_block_insns > M68KENG_MAX_BLOCK_INSNS ||
        !ops->name || !ops->domain_create || !ops->domain_destroy || !ops->invalidate || !ops->cpu_create ||
        !ops->cpu_destroy || !ops->get_reg || !ops->set_reg || !ops->run || !ops->get_stats)
        return "the engine's table is not one of interface 1.0";

    ctx->eng_host.size_of = sizeof(ctx->eng_host);
    ctx->eng_host.abi_major = M68KENG_ABI_MAJOR;
    ctx->eng_host.abi_minor = M68KENG_ABI_MINOR;
    ctx->eng_host.read = M68KEmu_EngineRead;
    ctx->eng_host.write = M68KEmu_EngineWrite;
    ctx->eng_host.log = eng_log;
    m68k_engine_map(ctx);

    ctx->eng_status = ops->domain_create(&ctx->eng_host, ctx, &ctx->eng_map, M68KENG_CAP_BASE, &domain);
    if (ctx->eng_status != M68KENG_OK || !domain)
        return ctx->eng_status == M68KENG_ERR_NOMEM  ? "the engine has no memory for the program" :
               ctx->eng_status == M68KENG_ERR_NOHOST ? "the engine cannot run on this machine" :
                                                    "the engine refused the program's memory";
    ctx->eng_ops = ops;
    ctx->eng_domain = domain;
    return NULL;
}

/* M68KEMU_CPU, read once before the program is loaded: unset or empty is
   the interpreter, as without this file; "interpreter" says so; "jit" is
   the engine or no run at all; "auto" is the engine when it is there and
   works. -1 when the program is not to run. */
static LONG eng_select(struct M68KEmuContext *ctx)
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
        m68k_engine_say(ctx, "M68KEMU_CPU starts \"%s\" and is too long: it can be interpreter, jit or auto; the program is not run", v);
        return -1;
    }
    /* its length, up to the end set above */
    for (n = 0; v[n]; n++)
        ;
    while (n > 0 && (v[n - 1] == '\n' || v[n - 1] == '\r' || v[n - 1] == ' '))
        v[--n] = 0;
    if (!n)
        return 0;

    if (!strcmp(v, "interpreter"))
    {
        ctx->eng_mode = M68KCPU_INTERPRETER;
        return 0;
    }
    if (!strcmp(v, "jit"))
        ctx->eng_mode = M68KCPU_ENGINE;
    else if (!strcmp(v, "auto"))
        ctx->eng_mode = M68KCPU_AUTO;
    else
    {
        m68k_engine_say(ctx, "M68KEMU_CPU is \"%s\": it can be interpreter, jit or auto; the program is not run", v);
        return -1;
    }

    why = eng_open(ctx);
    if (!why)
    {
        ctx->eng_run = TRUE;
        return 0;
    }
    m68k_engine_close(ctx);
    if (ctx->eng_mode == M68KCPU_ENGINE)
    {
        m68k_engine_say(ctx, "M68KEMU_CPU is jit and %s; the program is not run", why);
        return -1;
    }
    ctx->eng_why = why;
    return 0;
}

/* Without the variable the caller sees nothing of this, its IoErr() included */
LONG m68k_engine_select(struct M68KEmuContext *ctx)
{
    LONG err = IoErr(), r = eng_select(ctx);

    if (r == 0)
        SetIoErr(err);
    return r;
}

/* the program's name for a line: the end of it when it is a long path */
const char *m68k_engine_name(struct M68KEmuContext *ctx)
{
    ULONG len = 0;

    while (len < sizeof(ctx->program_name) - 1 && ctx->program_name[len])
        len++;
    if (!len)
        return "68k program";
    return ctx->program_name + (len > 48 ? len - 48 : 0);
}

/* The program wrote code, or the host did for it (a loader, a jump table, a
   stub, a cache call of the program): what the engine translated is void.
   Called by the task that holds the program's lock, or before any CPU. */
void m68k_engine_invalidate(struct M68KEmuContext *ctx)
{
    if (ctx->eng_domain)
        ctx->eng_ops->invalidate(ctx->eng_domain, 0, M68KENG_INVALIDATE_ALL);
}

/* after a run on the engine: what it did, and what the interpreter did */
void m68k_engine_report(struct M68KEmuContext *ctx)
{
    struct M68KEngStats s;

    if (!ctx->eng_domain || !ctx->eng_run)
        return;
    memset(&s, 0, sizeof(s));
    s.size_of = sizeof(s);
    if (ctx->eng_ops->get_stats(ctx->eng_domain, &s) != M68KENG_OK)
        return;
    m68k_engine_say(ctx, "%s: the engine ran %lu instructions in %lu runs (translated: %lu blocks, %lu instructions; "
                      "%lu invalidations, %lu code resets); the interpreter ran %lu",
        m68k_engine_name(ctx),
        (unsigned long)s.retired, (unsigned long)s.runs, (unsigned long)s.blocks_translated,
        (unsigned long)s.insns_translated, (unsigned long)s.invalidations, (unsigned long)s.code_resets,
        (unsigned long)ctx->eng_interp);
}

/* The end of the engine for this program: after its last CPU, before the
   program's memory is freed. The library is closed last. */
void m68k_engine_close(struct M68KEmuContext *ctx)
{
    if (ctx->eng_domain)
    {
        LONG st = ctx->eng_ops->domain_destroy(ctx->eng_domain);

        ctx->eng_domain = NULL;
        if (st != M68KENG_OK)
        {
            /* a CPU of the program is still there: the engine keeps what it
               holds, and so does the library */
            m68k_engine_say(ctx, "the engine kept its domain (status %ld)", (long)st);
            ctx->eng_base = NULL;
            ctx->eng_kept = TRUE;
        }
    }
    ctx->eng_ops = NULL;
    ctx->eng_run = FALSE;
    if (ctx->eng_base)
    {
        CloseLibrary(ctx->eng_base);
        ctx->eng_base = NULL;
    }
}

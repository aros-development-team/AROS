/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: m68kemu.library - native BPTR handles, VBL and EClock time,
          call ring dump, watchdog and log
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/bptr.h>
#include <aros/debug.h>
#include <string.h>
#include <stdio.h>

#include "m68kemu_intern.h"
#include "m68kemu_offsets.h"
#include "m68kemu_thunks.h"

/* ── Native BPTR handles ──────────────────────────────────────────────
   A native BPTR is a 64-bit pointer on this side and cannot travel through
   a 32-bit m68k register. The m68k side gets an index into a table instead;
   the same native BPTR always maps to the same handle. Values outside the
   handle range are m68k BPTRs into containment (BSTRs, in-memory seglists)
   and translate by address. */

ULONG m68k_bptr_out(struct M68KEmuContext *ctx, BPTR b)
{
    ULONG i, free_slot = (ULONG)-1;

    if (b == BNULL)
        return 0;
    if (!ctx->handles)
    {
        ctx->handles = AllocVec(M68KEMU_MAX_HANDLES * sizeof(BPTR), MEMF_CLEAR);
        if (!ctx->handles)
            return 0;
    }
    for (i = 0; i < ctx->num_handles; i++)
    {
        if (ctx->handles[i] == b)
            return M68KEMU_HANDLE_BASE + i * 4;
        if (ctx->handles[i] == BNULL && free_slot == (ULONG)-1)
            free_slot = i;
    }
    if (free_slot == (ULONG)-1)
    {
        if (ctx->num_handles >= M68KEMU_MAX_HANDLES)
        {
            bug("[m68kemu] out of BPTR handles\n");
            return 0;
        }
        free_slot = ctx->num_handles++;
    }
    ctx->handles[free_slot] = b;
    return M68KEMU_HANDLE_BASE + free_slot * 4;
}

BPTR m68k_bptr_in(struct M68KEmuContext *ctx, ULONG v)
{
    if (v == 0)
        return BNULL;
    if (v >= M68KEMU_HANDLE_BASE && v < M68KEMU_HANDLE_BASE + M68KEMU_MAX_HANDLES * 4)
    {
        ULONG i = (v - M68KEMU_HANDLE_BASE) / 4;
        if (ctx->handles && i < ctx->num_handles)
            return ctx->handles[i];
        return BNULL;
    }
    /* an m68k BPTR into containment */
    return MKBADDR(m68k_to_host(ctx, v << 2));
}

void m68k_bptr_drop(struct M68KEmuContext *ctx, ULONG v)
{
    if (v >= M68KEMU_HANDLE_BASE && v < M68KEMU_HANDLE_BASE + M68KEMU_MAX_HANDLES * 4)
    {
        ULONG i = (v - M68KEMU_HANDLE_BASE) / 4;
        if (ctx->handles && i < ctx->num_handles)
            ctx->handles[i] = BNULL;
    }
}

/* ── Vertical blank clock ────────────────────────────────────────────
   A 50 Hz tick from timer.device's E clock; the CPU loop asks every few
   thousand instructions. */

#include <devices/timer.h>
#include <proto/timer.h>

BOOL m68k_vbl_due(struct M68KEmuContext *ctx)
{
    struct EClockVal ev;
    struct Device *TimerBase;
    ULONG elapsed;

    if (!ctx->vbl_timerio)
    {
        /* only ReadEClock() is used, so the port needs no signal: it would
           belong to the task that made it, and another may free the context */
        struct MsgPort *port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
        struct timerequest *req = NULL;
        if (port)
        {
            port->mp_Node.ln_Type = NT_MSGPORT;
            port->mp_Flags = PA_IGNORE;
            NEWLIST(&port->mp_MsgList);
            req = (struct timerequest *)CreateIORequest(port, sizeof(struct timerequest));
        }
        if (!req || OpenDevice((CONST_STRPTR)"timer.device", UNIT_ECLOCK, (struct IORequest *)req, 0) != 0)
        {
            bug("[m68kemu] no timer.device for the vertical blank clock\n");
            if (req) DeleteIORequest((struct IORequest *)req);
            if (port) FreeMem(port, sizeof(struct MsgPort));
            return FALSE;
        }
        ctx->vbl_timerport = port;
        ctx->vbl_timerio = req;
        TimerBase = req->tr_node.io_Device;
        ctx->vbl_efreq = ReadEClock(&ev);
        ctx->vbl_last_lo = ev.ev_lo;
        ctx->vbl_last_hi = ev.ev_hi;
        return FALSE;
    }
    TimerBase = ((struct timerequest *)ctx->vbl_timerio)->tr_node.io_Device;
    ReadEClock(&ev);
    elapsed = ev.ev_lo - ctx->vbl_last_lo;          /* wraps correctly for short intervals */
    if (ctx->vbl_efreq == 0)
    {
        bug("[m68kemu] VBL clock: ReadEClock reports frequency 0\n");
        ctx->vbl_efreq = 709379;
    }
    {
        static ULONG calls = 0;
        if ((++calls & 0x3FFF) == 0 && ctx->trace_calls)
            bug("[m68kemu] VBL clock: call %lu elapsed %lu ev_lo %lu last %lu\n", (unsigned long)calls,
                (unsigned long)elapsed, (unsigned long)ev.ev_lo, (unsigned long)ctx->vbl_last_lo);
    }
    if (elapsed < ctx->vbl_efreq / 50)
        return FALSE;
    if (ctx->vbl_count < 8 && ctx->trace_calls)
        bug("[m68kemu] VBL clock: due, elapsed %lu E ticks (freq %lu)\n", (unsigned long)elapsed, (unsigned long)ctx->vbl_efreq);
    ctx->vbl_last_lo = ev.ev_lo;
    ctx->vbl_last_hi = ev.ev_hi;
    return TRUE;
}

void m68k_vbl_close(struct M68KEmuContext *ctx)
{
    if (ctx->vbl_timerio)
    {
        CloseDevice((struct IORequest *)ctx->vbl_timerio);
        DeleteIORequest((struct IORequest *)ctx->vbl_timerio);
        ctx->vbl_timerio = NULL;
    }
    if (ctx->vbl_timerport)
    {
        FreeMem(ctx->vbl_timerport, sizeof(struct MsgPort));
        ctx->vbl_timerport = NULL;
    }
}

/* Current E clock (ev_lo) for the CIA model; opens the timer like m68k_vbl_due */
ULONG m68k_eclock_now(struct M68KEmuContext *ctx, ULONG *freq)
{
    struct EClockVal ev;
    struct Device *TimerBase;

    if (!ctx->vbl_timerio)
    {
        (void)m68k_vbl_due(ctx);
        if (!ctx->vbl_timerio)
        {
            if (freq) *freq = 709379;
            return 0;
        }
    }
    TimerBase = ((struct timerequest *)ctx->vbl_timerio)->tr_node.io_Device;
    ReadEClock(&ev);
    if (freq) *freq = ctx->vbl_efreq ? ctx->vbl_efreq : 709379;
    return ev.ev_lo;
}

/* the last library calls (M68KEMU_TRACE=end), string compares only counted:
   a loop that walks a list hides the calls that drive it */
void m68k_dump_callring(struct M68KEmuContext *ctx)
{
    ULONG n = ctx->callring_pos < 2048 ? ctx->callring_pos : 2048, cmp = 0, shown = 0;

    for (ULONG i = ctx->callring_pos - n; i < ctx->callring_pos && shown < 200; i++)
    {
        ULONG k = i & 2047;
        if (!strcmp(ctx->callring[k].lib, "utility.library") &&
            (ctx->callring[k].lvo == 162 || ctx->callring[k].lvo == 168))
        {
            cmp++;
            continue;
        }
        if (cmp)
            bug("[m68kemu]   (%lu string compares)\n", (unsigned long)cmp);
        cmp = 0;
        shown++;
        bug("[m68kemu]   %s -%lu d0 %08lx d1 %08lx a0 %08lx a1 %08lx -> %08lx (from %08lx)\n",
            ctx->callring[k].lib, (unsigned long)ctx->callring[k].lvo,
            (unsigned long)ctx->callring[k].d0, (unsigned long)ctx->callring[k].d1,
            (unsigned long)ctx->callring[k].a0, (unsigned long)ctx->callring[k].a1,
            (unsigned long)ctx->callring[k].ret, (unsigned long)ctx->callring[k].from);
    }
    if (cmp)
        bug("[m68kemu]   (%lu string compares)\n", (unsigned long)cmp);
}

/* Diagnostic watchdog (M68KEMU_TRACE set): a native process that prints
   where the emulated CPU is every two seconds, so a stall inside one
   instruction shows up as a frozen count and PC */
#include <dos/dostags.h>
static struct M68KEmuContext *wd_ctx;
static void wd_entry(void)
{
    struct M68KEmuContext *ctx = wd_ctx;
    ULONG last = 0xFFFFFFFF, stuck = 0, ticks = 0;
    while (!ctx->watchdog_quit)
    {
        ULONG ic, pc;
        Delay(100);
        if (ctx->watchdog_quit) break;
        /* M68KEMU_TRACE=end: once the program has sat in one call for ten
           seconds, the calls that led there */
        if (ctx->trace_end && !ctx->trace_calls)
        {
            ic = ctx->icount_lo;
            stuck = ic == last ? stuck + 1 : 0;
            last = ic;
            if (stuck == 5 || ++ticks == 30)
            {
                bug("[m68kemu] %s %s, after %lu calls:\n",
                    ctx->program_name[0] ? ctx->program_name : "68k program",
                    stuck == 5 ? "has been in one library call for 10 s" : "is still running at 60 s",
                    (unsigned long)ctx->callring_pos);
                m68k_dump_callring(ctx);
                pc = ctx->pc_ring[(ctx->pc_ring_pos - 1) & 511];
                bug("[m68kemu]   now at pc %08lx (%04x %04x)\n", (unsigned long)pc,
                    (unsigned)m68k_read16(ctx, pc), (unsigned)m68k_read16(ctx, pc + 2));
                for (UWORD l = 0; l < ctx->num_libs; l++)
                    if (pc >= ctx->libs[l].jt_start && pc < ctx->libs[l].m68k_addr)
                        bug("[m68kemu]   in %s -%lu, called from %08lx\n", ctx->libs[l].name[0] ? ctx->libs[l].name : "(library)",
                            (unsigned long)(ctx->libs[l].m68k_addr - pc), (unsigned long)ctx->pc_ring[(ctx->pc_ring_pos - 2) & 511]);
            }
            continue;
        }
        if (!ctx->trace_calls) continue;           /* quiet until the trace starts */
        ic = ctx->icount_lo;
        pc = ctx->pc_ring[(ctx->pc_ring_pos - 1) & 511];
        bug("[m68kemu] WATCHDOG icount %lu%s pc %08lx op %04x %04x vbl %lu irq %d code %08lx\n",
            (unsigned long)ic, ic == last ? " (STUCK)" : "", (unsigned long)pc,
            (unsigned)m68k_read16(ctx, pc), (unsigned)m68k_read16(ctx, pc + 2),
            (unsigned long)ctx->vbl_count, (int)ctx->vbl_irq, (unsigned long)ctx->vbl_code);
        last = ic;
    }
    ctx->watchdog_quit = FALSE;
}

void m68k_watchdog_start(struct M68KEmuContext *ctx)
{
    wd_ctx = ctx;
    ctx->watchdog_quit = FALSE;
    ctx->watchdog_on = CreateNewProcTags(NP_Entry, (IPTR)wd_entry, NP_Name, (IPTR)"m68kemu watchdog",
                                         NP_Priority, 5, TAG_DONE) != NULL;
}

void m68k_watchdog_stop(struct M68KEmuContext *ctx)
{
    if (!ctx->watchdog_on) return;
    ctx->watchdog_quit = TRUE;
    while (ctx->watchdog_quit) Delay(10);
    ctx->watchdog_on = FALSE;
}

/* Messages about a program (it failed, a library is missing): to the
   system log, which LogDump and the log viewers read, so nothing paints
   over the screen; to the debug output as well while M68KEMU_TRACE is set. */
#include <resources/log.h>
static struct Library *m68k_LogResBase;
#define __LOG_LIBBASE m68k_LogResBase
#include <proto/log.h>
#include <stdarg.h>
#include <stdio.h>
static APTR m68k_loghandle;
static struct Node m68k_lognode = { .ln_Name = (char *)"m68kemu" };

void m68k_log(struct M68KEmuContext *ctx, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (ctx && ctx->trace_calls)
        bug("[m68kemu] %s\n", buf);
    Forbid();
    if (!m68k_LogResBase)
    {
        m68k_LogResBase = OpenResource("log.resource");
        if (m68k_LogResBase)
            m68k_loghandle = logInitialise(&m68k_lognode);
    }
    Permit();
    if (m68k_LogResBase && m68k_loghandle)
    {
        /* the message is already formatted: plain text, no format arguments */
        char *c;
        for (c = buf; *c; c++)
            if (*c == '%') *c = '?';
        logAddEntryA(LOGF_Flag_Type_Error | 20, m68k_loghandle, (CONST_STRPTR)"", (CONST_STRPTR)"m68kemu", 0,
                     (CONST_STRPTR)buf, (RAWARG)NULL);
    }
    else if (!ctx || !ctx->trace_calls)
        bug("[m68kemu] %s\n", buf);
}

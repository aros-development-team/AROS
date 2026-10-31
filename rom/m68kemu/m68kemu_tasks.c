/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library — more than one task in a 68k program: its processes,
    and the messages they and other programs send each other.

    Every task of the program runs its own CPU over the program's memory.
    The emulator's state is shared, so a CPU runs 68k code only while it
    holds the program's lock; it lets go in calls that wait, and now and
    then between instructions, so the tasks take turns as on AmigaOS. What
    a task keeps for its own calls (its CPUs, a thunk's tail call, the
    methods it passes on) is in its M68KTaskState. The program's own
    semaphores work on its SignalSemaphore structures, under the lock.

    A message the program makes lives in its memory in 68k layout. Sent to
    a port in its memory, it is queued there as exec queues it, and the
    port's task is signalled. Sent to a native port (CreateMsgPort, a
    process's pr_MsgPort), it travels in a small native carrier; GetMsg
    hands the program its own message again.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"
#include "m68kemu_offsets.h"

void *shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr);

/* ---------------------------------------------------------------------- */
/* the tasks of the program                                               */

struct M68KTaskState *m68k_ts_find(struct M68KEmuContext *ctx, struct Task *task)
{
    struct M68KTaskState *ts, *found = NULL;

    for (int i = 0; i < M68KEMU_MAX_TASKS; i++)
        if (ctx->tasks[i].task == task)
            return &ctx->tasks[i];
    Forbid();
    ForeachNode(&ctx->more_tasks, ts)
        if (ts->task == task)
        {
            found = ts;
            break;
        }
    Permit();
    return found;
}

struct M68KTaskState *m68k_ts(struct M68KEmuContext *ctx)
{
    struct Task *me = FindTask(NULL);
    struct M68KTaskState *ts;

    /* before the program runs, its setup (the libraries its startup opens)
       is done on behalf of its main task, whatever task does it */
    if (!ctx->emu_task)
        return &ctx->tasks[0];
    if ((ts = m68k_ts_find(ctx, me)))
        return ts;
    Forbid();
    for (int i = 1; i < M68KEMU_MAX_TASKS && !ts; i++)
        if (!ctx->tasks[i].task)
        {
            ts = &ctx->tasks[i];
            memset(ts, 0, sizeof(*ts));
            ts->task = me;
        }
    if (!ts && (ts = AllocMem(sizeof(*ts), MEMF_CLEAR)))
    {
        ts->task = me;
        AddTail((struct List *)&ctx->more_tasks, (struct Node *)ts);
    }
    Permit();
    if (!ts)
        return &ctx->orphan;
    ts->semsig = AllocSignal(-1);
    ts->running = TRUE;
    return ts;
}

/* the main task takes over the state its setup used */
void m68k_ts_main(struct M68KEmuContext *ctx)
{
    ctx->emu_task = FindTask(NULL);
    ctx->tasks[0].task = ctx->emu_task;
    ctx->tasks[0].semsig = AllocSignal(-1);
    ctx->tasks[0].running = TRUE;
}

/* this task is done with the program: its CPUs and signal go */
void m68k_ts_drop(struct M68KEmuContext *ctx)
{
    struct M68KTaskState *ts = m68k_ts_find(ctx, FindTask(NULL));

    if (!ts)
        return;
    M68KEmu_FreeNestCpus(ctx, ts);
    if (ts->semsig >= 0)
        FreeSignal(ts->semsig);
    for (int i = 0; i < ts->nstash; i++)
        FreeVec(ts->stash[i]);
    ts->task = NULL;
    if ((IPTR)ts < (IPTR)ctx->tasks || (IPTR)ts >= (IPTR)(ctx->tasks + M68KEMU_MAX_TASKS))
    {
        Forbid();
        Remove((struct Node *)ts);
        Permit();
        FreeMem(ts, sizeof(*ts));
    }
}

/* The context lives as long as any of the program's tasks, or a native
   program it started, still uses it; the last one frees it. The library
   stays open with them: a process of the program runs its code. */
void m68k_context_hold(struct M68KEmuContext *ctx)
{
    Forbid();
    ctx->alive++;
    ctx->emuBase->lib.lib_OpenCnt++;
    Permit();
}

void m68k_context_release(struct M68KEmuContext *ctx)
{
    struct Library *lib = &ctx->emuBase->lib;
    BOOL last;

    Forbid();
    last = --ctx->alive == 0;
    Permit();
    if (last)
        M68KEmu_DestroyContext(ctx);
    Forbid();
    lib->lib_OpenCnt--;
    Permit();
}

/* the program ends: on an error, its processes end with it */
void m68k_program_stop(struct M68KEmuContext *ctx, BOOL fault)
{
    ctx->running = FALSE;
    if (fault)
        ctx->faulted = TRUE;
}

/* ---------------------------------------------------------------------- */
/* the program's lock                                                     */

void m68k_gil_init(struct M68KEmuContext *ctx)
{
    InitSemaphore(&ctx->gil);
}

/* As exec keeps a task's Forbid() and Disable() nesting while it waits, a
   task of the program that lets go of the lock takes its nesting with it:
   the tasks that run meanwhile are not forbidden by it, and it has its
   nesting back when it holds the lock again */
void m68k_gil_obtain(struct M68KEmuContext *ctx)
{
    struct M68KTaskState *ts;

    ObtainSemaphore(&ctx->gil);
    ts = m68k_ts(ctx);
    ctx->forbid = ts->forbid;
    ctx->disable = ts->disable;
    ts->forbid = ts->disable = 0;
}

void m68k_gil_release(struct M68KEmuContext *ctx)
{
    struct M68KTaskState *ts;

    if (ctx->gil.ss_Owner != FindTask(NULL))
        return;
    if ((ts = m68k_ts_find(ctx, FindTask(NULL))))     /* none once the task has left */
    {
        ts->forbid = ctx->forbid;
        ts->disable = ctx->disable;
    }
    ctx->forbid = ctx->disable = 0;
    ReleaseSemaphore(&ctx->gil);
}

/* between instructions: let another task of the program run, unless the
   program has forbidden task switching */
void m68k_gil_yield(struct M68KEmuContext *ctx)
{
    if (ctx->forbid || ctx->disable || ctx->gil.ss_QueueCount <= 0)   /* nobody waiting */
        return;
    m68k_gil_release(ctx);
    m68k_gil_obtain(ctx);
}

/* ---------------------------------------------------------------------- */
/* native memory that stays with the program                              */

APTR m68k_gfx_own(struct M68KEmuContext *ctx, ULONG size)
{
    struct Node *n = AllocVec(sizeof(struct Node) + size, MEMF_CLEAR);

    if (!n)
        return NULL;
    AddTail(&ctx->gfx_owned, n);
    return n + 1;
}

/* an allocation made elsewhere that is to stay with the program */
APTR m68k_keep_forever(struct M68KEmuContext *ctx, APTR p)
{
    APTR *w = p ? m68k_gfx_own(ctx, sizeof(APTR)) : NULL;

    if (w)
    {
        *w = p;
        ((struct Node *)w - 1)->ln_Type = 1;    /* frees what it points to as well */
    }
    return p;
}

void m68k_gfx_disown(struct M68KEmuContext *ctx, APTR p)
{
    struct Node *n = (struct Node *)p - 1, *i;

    if (!p)
        return;
    ForeachNode(&ctx->gfx_owned, i)
        if (i == n)
        {
            Remove(n);
            FreeVec(n);
            return;
        }
}

/* ---------------------------------------------------------------------- */
/* the program's semaphores                                               */
/* struct SignalSemaphore in its memory: ss_Link 0, ss_NestCount 14,
   ss_WaitQueue 16, ss_MultipleLink 28, ss_Owner 40, ss_QueueCount 44;
   struct SemaphoreRequest: sr_Link 0, sr_Waiter 8. All of this runs under
   the program's lock, so the fields change as if in Forbid(). A waiter's
   sr_Waiter has bit 0 set for a shared request, and is cleared when the
   semaphore was handed to it. */

#define LN_TYPE       8
#define SS_NESTCOUNT  14
#define SS_WAITQUEUE  16
#define SS_OWNER      40
#define SS_QUEUECOUNT 44
#define SR_WAITER     8
#define SR_SIZE       12

static void list_addtail(struct M68KEmuContext *ctx, ULONG list, ULONG node);
static ULONG list_remhead(struct M68KEmuContext *ctx, ULONG list);

static WORD sem_count(struct M68KEmuContext *ctx, ULONG ss, ULONG off, WORD add)
{
    WORD v = (WORD)m68k_read16(ctx, ss + off) + add;
    m68k_write16(ctx, ss + off, (UWORD)v);
    return v;
}

static ULONG sem_me(struct M68KEmuContext *ctx)
{
    return m68k_process_of(ctx, FindTask(NULL));
}

/* the semaphore is handed to the requests at the head of the queue: one
   exclusive, or every shared one in a row */
static void sem_handover(struct M68KEmuContext *ctx, ULONG ss)
{
    ULONG head = m68k_read32(ctx, ss + SS_WAITQUEUE), req;
    WORD granted = 0;

    m68k_write32(ctx, ss + SS_OWNER, 0);
    if (!m68k_in_mem(ctx, head, SR_SIZE) || !m68k_read32(ctx, head))
    {
        m68k_write16(ctx, ss + SS_NESTCOUNT, 0);
        return;
    }
    while ((head = m68k_read32(ctx, ss + SS_WAITQUEUE)) && m68k_read32(ctx, head))
    {
        ULONG waiter = m68k_read32(ctx, head + SR_WAITER);
        BOOL shared = (waiter & 1) != 0;
        struct Task *t;
        struct M68KTaskState *ts;

        if (granted && !shared)
            break;
        req = list_remhead(ctx, ss + SS_WAITQUEUE);
        if (!shared)
            m68k_write32(ctx, ss + SS_OWNER, waiter);
        m68k_write32(ctx, req + SR_WAITER, 0);
        granted++;
        if ((t = (struct Task *)shadow_lookup(ctx, waiter & ~1UL)) && (ts = m68k_ts_find(ctx, t)) && ts->semsig >= 0)
            Signal(t, 1UL << ts->semsig);
        if (!shared)
            break;
    }
    m68k_write16(ctx, ss + SS_NESTCOUNT, (UWORD)granted);
}

/* queue a request and wait, letting the program's other tasks run */
static void sem_wait(struct M68KEmuContext *ctx, ULONG ss, BOOL shared)
{
    struct M68KTaskState *ts = m68k_ts(ctx);
    ULONG req = M68KEmu_HeapAlloc(ctx, SR_SIZE, MEMF_CLEAR);

    if (!req)
        return;
    m68k_write32(ctx, req + SR_WAITER, sem_me(ctx) | (shared ? 1 : 0));
    list_addtail(ctx, ss + SS_WAITQUEUE, req);
    while (m68k_read32(ctx, req + SR_WAITER))
    {
        m68k_gil_release(ctx);
        if (ts->semsig >= 0)
            Wait(1UL << ts->semsig);
        else
            Delay(1);
        m68k_gil_obtain(ctx);
    }
    M68KEmu_HeapFree(ctx, req, SR_SIZE);
}

/* a native semaphore the program found: the lock is let go while it waits */
static BOOL sem_native(struct M68KEmuContext *ctx, ULONG ss, struct SignalSemaphore **out)
{
    if (m68k_in_mem(ctx, ss, 46) && !shadow_lookup(ctx, ss))
        return FALSE;
    *out = (struct SignalSemaphore *)m68k_to_host_or_shadow(ctx, ss);
    return TRUE;
}

void m68k_sem_init(struct M68KEmuContext *ctx, ULONG ss)
{
    if (!m68k_in_mem(ctx, ss, 46))
        return;
    ctx->mem[ss + LN_TYPE] = NT_SIGNALSEM;
    m68k_write16(ctx, ss + SS_NESTCOUNT, 0);
    m68k_write32(ctx, ss + SS_WAITQUEUE, ss + SS_WAITQUEUE + 4);
    m68k_write32(ctx, ss + SS_WAITQUEUE + 4, 0);
    m68k_write32(ctx, ss + SS_WAITQUEUE + 8, ss + SS_WAITQUEUE);
    m68k_write32(ctx, ss + SS_OWNER, 0);
    m68k_write16(ctx, ss + SS_QUEUECOUNT, (UWORD)-1);
}

/* ObtainSemaphore / ObtainSemaphoreShared; attempt: TRUE to give up instead of waiting */
BOOL m68k_sem_obtain(struct M68KEmuContext *ctx, ULONG ss, BOOL shared, BOOL attempt)
{
    struct SignalSemaphore *nss;
    ULONG me, owner, head;
    BOOL free, queued;

    if (sem_native(ctx, ss, &nss))
    {
        if (!nss)
            return FALSE;
        if (attempt)
            return shared ? AttemptSemaphoreShared(nss) : AttemptSemaphore(nss);
        m68k_gil_release(ctx);
        if (shared) ObtainSemaphoreShared(nss); else ObtainSemaphore(nss);
        m68k_gil_obtain(ctx);
        return TRUE;
    }
    me = sem_me(ctx);
    owner = m68k_read32(ctx, ss + SS_OWNER);
    head = m68k_read32(ctx, ss + SS_WAITQUEUE);
    free = (WORD)m68k_read16(ctx, ss + SS_QUEUECOUNT) < 0;
    queued = m68k_in_mem(ctx, head, SR_SIZE) && m68k_read32(ctx, head) != 0;
    if (free || owner == me || (shared && !owner && !queued))
    {
        sem_count(ctx, ss, SS_QUEUECOUNT, 1);
        sem_count(ctx, ss, SS_NESTCOUNT, 1);
        if (free)
            m68k_write32(ctx, ss + SS_OWNER, shared ? 0 : me);
        return TRUE;
    }
    if (attempt)
        return FALSE;
    sem_count(ctx, ss, SS_QUEUECOUNT, 1);
    sem_wait(ctx, ss, shared);
    return TRUE;
}

void m68k_sem_release(struct M68KEmuContext *ctx, ULONG ss)
{
    struct SignalSemaphore *nss;

    if (sem_native(ctx, ss, &nss))
    {
        if (nss)
            ReleaseSemaphore(nss);
        return;
    }
    if ((WORD)m68k_read16(ctx, ss + SS_QUEUECOUNT) < 0)
        return;
    sem_count(ctx, ss, SS_QUEUECOUNT, -1);
    if (sem_count(ctx, ss, SS_NESTCOUNT, -1) > 0)
        return;
    sem_handover(ctx, ss);
}

/* ObtainSemaphoreList / ReleaseSemaphoreList: the semaphores on a list of
   the program's, one after the other */
void m68k_sem_list(struct M68KEmuContext *ctx, ULONG list, BOOL obtain)
{
    ULONG n, guard = 0;

    if (!m68k_in_mem(ctx, list, 12))
        return;
    for (n = m68k_read32(ctx, list); m68k_in_mem(ctx, n, 46) && m68k_read32(ctx, n) && guard++ < 1024; n = m68k_read32(ctx, n))
        if (obtain)
            m68k_sem_obtain(ctx, n, FALSE, FALSE);
        else
            m68k_sem_release(ctx, n);
}

/* ---------------------------------------------------------------------- */
/* exec lists in the program's memory                                     */
/* struct List: lh_Head 0, lh_Tail 4, lh_TailPred 8; struct MsgPort:
   mp_Flags 14, mp_SigBit 15, mp_SigTask 16, mp_MsgList 20 */

#define MP_FLAGS   14
#define MP_SIGBIT  15
#define MP_SIGTASK 16
#define MP_LIST    20
#define MN_REPLY   14

static void list_addtail(struct M68KEmuContext *ctx, ULONG list, ULONG node)
{
    ULONG pred = m68k_read32(ctx, list + 8);

    m68k_write32(ctx, node, list + 4);
    m68k_write32(ctx, node + 4, pred);
    m68k_write32(ctx, pred, node);
    m68k_write32(ctx, list + 8, node);
}

static ULONG list_remhead(struct M68KEmuContext *ctx, ULONG list)
{
    ULONG head = m68k_read32(ctx, list), succ;

    if (!m68k_in_mem(ctx, head, 8) || !(succ = m68k_read32(ctx, head)))
        return 0;
    m68k_write32(ctx, list, succ);
    m68k_write32(ctx, succ + 4, list);
    return head;
}

/* a port in the program's memory: one without a native port behind it */
BOOL m68k_port_is_68k(struct M68KEmuContext *ctx, ULONG mport)
{
    return m68k_in_mem(ctx, mport, 34) && !shadow_lookup(ctx, mport);
}

ULONG m68k_port_getmsg(struct M68KEmuContext *ctx, ULONG mport)
{
    return list_remhead(ctx, mport + MP_LIST);
}

/* WaitPort on a port in the program's memory */
ULONG m68k_port_wait(struct M68KEmuContext *ctx, ULONG mport)
{
    ULONG head;

    while (!(head = m68k_read32(ctx, mport + MP_LIST)) || !m68k_read32(ctx, head))
    {
        UBYTE bit = ctx->mem[mport + MP_SIGBIT];
        m68k_gil_release(ctx);
        Wait(1UL << (bit & 31));
        m68k_gil_obtain(ctx);
    }
    return head;
}

/* ---------------------------------------------------------------------- */
/* sending                                                                */

static const char carrier_name[] = "m68kemu carrier";

struct M68KCarrier
{
    struct Message msg;
    ULONG mmsg;
    UBYTE type;
};

/* A WBStartup for a native program the program started from a seglist, as
   Workbench replacements start the WBStartup drawer and icons: the native
   program reads it, and replies to the twin of the program's reply port.
   m68k struct WBStartup: sm_Process 20, sm_Segment 24, sm_NumArgs 28,
   sm_ToolWindow 32, sm_ArgList 36; struct WBArg: wa_Lock 0, wa_Name 4 */
static const char wbcarrier_name[] = "m68kemu wbstartup";

struct M68KWBCarrier
{
    struct WBStartup wbs;
    ULONG mmsg;
    UBYTE type;
    struct WBArg args[];
};

static BOOL deliver_wbstartup(struct M68KEmuContext *ctx, struct MsgPort *port, ULONG mmsg)
{
    struct M68KWBCarrier *c;
    struct MsgPort *reply = NULL;
    ULONG nargs, margs, i, mreply;
    BOOL own = FALSE;
    int k;

    for (k = 0; k < 8; k++)
        if (ctx->native_procs[k] && &((struct Process *)ctx->native_procs[k])->pr_MsgPort == port)
            break;
    if (k == 8 || !m68k_in_mem(ctx, mmsg, 40))
        return FALSE;
    nargs = m68k_read32(ctx, mmsg + 28);
    margs = m68k_read32(ctx, mmsg + 36);
    if (nargs > 64 || (nargs && !m68k_in_mem(ctx, margs, nargs * 8)))
        nargs = 0;
    if (!(c = AllocVec(sizeof(*c) + nargs * sizeof(struct WBArg), MEMF_PUBLIC | MEMF_CLEAR)))
        return FALSE;
    if ((mreply = m68k_read32(ctx, mmsg + MN_REPLY)))
        reply = m68k_port_twin(ctx, mreply, &own);
    c->wbs.sm_Message.mn_Node.ln_Type = NT_MESSAGE;
    c->wbs.sm_Message.mn_Node.ln_Name = (STRPTR)wbcarrier_name;
    c->wbs.sm_Message.mn_Length = sizeof(struct WBStartup);
    c->wbs.sm_Message.mn_ReplyPort = reply;
    c->wbs.sm_Process = port;
    c->wbs.sm_Segment = m68k_bptr_in(ctx, m68k_read32(ctx, mmsg + 24));
    c->wbs.sm_NumArgs = nargs;
    c->wbs.sm_ToolWindow = m68k_read32(ctx, mmsg + 32) ? (char *)m68k_to_host(ctx, m68k_read32(ctx, mmsg + 32)) : NULL;
    c->wbs.sm_ArgList = nargs ? c->args : NULL;
    for (i = 0; i < nargs; i++)
    {
        ULONG name = m68k_read32(ctx, margs + i * 8 + 4);
        c->args[i].wa_Lock = m68k_bptr_in(ctx, m68k_read32(ctx, margs + i * 8));
        c->args[i].wa_Name = name ? (STRPTR)m68k_to_host(ctx, name) : NULL;
    }
    c->mmsg = mmsg;
    c->type = NT_REPLYMSG;
    ctx->native_procs[k] = NULL;
    PutMsg(port, &c->wbs.sm_Message);
    return TRUE;
}

void m68k_deliver(struct M68KEmuContext *ctx, ULONG mport, ULONG mmsg, UBYTE type)
{
    struct MsgPort *port;

    if (!m68k_in_mem(ctx, mmsg, 20))
        return;
    ctx->mem[mmsg + LN_TYPE] = type;
    if ((port = (struct MsgPort *)shadow_lookup(ctx, mport)))
    {
        struct M68KCarrier *c;

        if (type == NT_MESSAGE && deliver_wbstartup(ctx, port, mmsg))
            return;
        c = AllocVec(sizeof(*c), MEMF_PUBLIC | MEMF_CLEAR);
        if (!c)
            return;
        c->msg.mn_Node.ln_Name = (STRPTR)carrier_name;
        c->msg.mn_Length = sizeof(*c);
        c->mmsg = mmsg;
        c->type = type;
        PutMsg(port, &c->msg);
        return;
    }
    if (!m68k_in_mem(ctx, mport, 34))
        return;
    list_addtail(ctx, mport + MP_LIST, mmsg);
    if ((ctx->mem[mport + MP_FLAGS] & PF_ACTION) == PA_SIGNAL)
    {
        struct Task *t = (struct Task *)shadow_lookup(ctx, m68k_read32(ctx, mport + MP_SIGTASK));
        if (t)
            Signal(t, 1UL << (ctx->mem[mport + MP_SIGBIT] & 31));
    }
}

/* a native message from a native port: the program's own, if it came in a carrier */
ULONG m68k_msg_from_native(struct M68KEmuContext *ctx, struct Message *m)
{
    struct M68KCarrier *c = (struct M68KCarrier *)m;
    ULONG mmsg;

    if (m && (const char *)m->mn_Node.ln_Name == wbcarrier_name)
    {
        struct M68KWBCarrier *w = (struct M68KWBCarrier *)m;

        mmsg = w->mmsg;
        ctx->mem[mmsg + LN_TYPE] = NT_REPLYMSG;
        FreeVec(w);
        return mmsg;
    }
    if (!m || (const char *)m->mn_Node.ln_Name != carrier_name)
        return 0;
    mmsg = c->mmsg;
    ctx->mem[mmsg + LN_TYPE] = c->type;
    FreeVec(c);
    return mmsg;
}

/* ReplyMsg of a message in the program's memory */
void m68k_reply(struct M68KEmuContext *ctx, ULONG mmsg)
{
    ULONG rp;

    if (!m68k_in_mem(ctx, mmsg, 20))
        return;
    if (ctx->trace_calls)
        bug("[m68kemu] ReplyMsg %08lx to port %08lx (%s)\n", (unsigned long)mmsg, (unsigned long)m68k_read32(ctx, mmsg + MN_REPLY),
            shadow_lookup(ctx, m68k_read32(ctx, mmsg + MN_REPLY)) ? "native" : "in memory");
    if ((rp = m68k_read32(ctx, mmsg + MN_REPLY)))
        m68k_deliver(ctx, rp, mmsg, NT_REPLYMSG);
    else
        ctx->mem[mmsg + LN_TYPE] = NT_FREEMSG;
}

/* ---------------------------------------------------------------------- */
/* the program's processes                                                */

struct M68KChild
{
    struct M68KEmuContext *ctx;
    ULONG entry, stack, stacksize, exitcode, exitdata;
    struct Task *parent;
};

static LONG child_entry(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    struct M68KChild *ch = (struct M68KChild *)me->pr_Task.tc_UserData;
    struct M68KEmuContext *ctx = ch->ctx;
    ULONG regs[15] = { 0 };
    LONG rc;

    m68k_gil_obtain(ctx);
    m68k_process_of(ctx, &me->pr_Task);
    regs[14] = ctx->m68k_sysbase;              /* A6 */
    rc = (LONG)M68KEmu_RunTask(ctx, ch->entry, ch->stack + ch->stacksize, regs);
    if (ch->exitcode && !ctx->faulted)
    {
        regs[0] = rc;
        regs[1] = ch->exitdata;
        M68KEmu_RunTask(ctx, ch->exitcode, ch->stack + ch->stacksize, regs);
    }
    M68KEmu_HeapFree(ctx, ch->stack, ch->stacksize);
    ctx->num_children--;
    m68k_ts_drop(ctx);
    m68k_gil_release(ctx);
    FreeVec(ch);
    m68k_context_release(ctx);
    return rc;
}

/* a native program the program started ended: its slot and its hold go */
void m68k_native_proc_exit(IPTR rc, IPTR data)
{
    struct M68KEmuContext *ctx = (struct M68KEmuContext *)data;
    struct Task *me = FindTask(NULL);

    (void)rc;
    for (int k = 0; k < 8; k++)
        if (ctx->native_procs[k] == me)
            ctx->native_procs[k] = NULL;
    m68k_context_release(ctx);
}

/* dos -498: CreateNewProc(D1=tags): NP_Entry is 68k code of the program */
IPTR m68k_dos_CreateNewProc(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG state = M68KEmu_GetD(cpu, 1), ti;
    struct M68KChild *ch;
    struct TagItem t[20];
    struct Process *p;
    char *name = NULL;
    int n = 0;
    ULONG seglist = 0;

    if (!(ch = AllocVec(sizeof(*ch), MEMF_CLEAR)))
        return 0;
    ch->ctx = ctx;
    ch->stacksize = 16384;
    while ((ti = m68k_next_tag(ctx, &state)) && n < 12)
    {
        ULONG tag = m68k_read32(ctx, ti), data = m68k_read32(ctx, ti + 4);

        switch (tag)
        {
        case NP_Entry:      ch->entry = data; break;
        case NP_Seglist:    seglist = data; break;
        case NP_StackSize:  if (data > ch->stacksize) ch->stacksize = (data + 15) & ~15; break;
        case NP_ExitCode:   ch->exitcode = data; break;
        case NP_ExitData:   ch->exitdata = data; break;
        case NP_Priority:   t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)(LONG)data; break;
        case NP_Name:
            if (m68k_in_mem(ctx, data, 1))
            {
                ULONG len = strnlen((char *)ctx->mem + data, ctx->mem_size - data);
                if (!(name = AllocVec(len + 1, MEMF_ANY)))
                    break;
                memcpy(name, ctx->mem + data, len);
                name[len] = 0;
                t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)name;
            }
            break;
        case NP_Input:
        case NP_Output:
        case NP_Error:
        case NP_CurrentDir:
        case NP_HomeDir:
            t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)m68k_bptr_in(ctx, data);
            break;
        case NP_CloseInput:
        case NP_CloseOutput:
        case NP_CloseError:
        case NP_Cli:
        case NP_FreeSeglist:
            t[n].ti_Tag = tag; t[n++].ti_Data = data;
            break;
        }
    }
    /* a program LoadSeg() gave it: a 68k one runs in this one's memory, a
       native one (Directory Opus starting the WBStartup programs) natively */
    if (!ch->entry && seglist)
    {
        BPTR seg = m68k_bptr_in(ctx, seglist);
        IPTR hunk = 0;
        struct TagItem gt[] = { { GSLI_68KHUNK, (IPTR)&hunk }, { TAG_DONE, 0 } };

        if (seg && (!GetSegListInfo(seg, gt) || !hunk))
        {
            ULONG stack = ch->stacksize > 65536 ? ch->stacksize : 65536;
            int k;

            FreeVec(ch);
            for (k = 0; k < 8 && ctx->native_procs[k]; k++)
                ;
            if (k == 8)
            {
                FreeVec(name);
                return 0;
            }
            t[n].ti_Tag = NP_Seglist;   t[n++].ti_Data = (IPTR)seg;
            t[n].ti_Tag = NP_StackSize; t[n++].ti_Data = stack;
            t[n].ti_Tag = NP_ExitCode;  t[n++].ti_Data = (IPTR)m68k_native_proc_exit;
            t[n].ti_Tag = NP_ExitData;  t[n++].ti_Data = (IPTR)ctx;
            t[n].ti_Tag = TAG_DONE;
            m68k_context_hold(ctx);
            Forbid();
            p = CreateNewProc(t);
            if (p)
                ctx->native_procs[k] = p;
            Permit();
            FreeVec(name);
            if (!p)
            {
                m68k_context_release(ctx);
                return 0;
            }
            return m68k_process_of(ctx, &p->pr_Task);
        }
        if (seg)
        {
            ULONG ep = ctx->entry_point, ss = ctx->seg_start;
            ch->entry = M68KEmu_LoadHunks(ctx, seg);
            ctx->entry_point = ep;
            ctx->seg_start = ss;
        }
    }
    if (!ch->entry || !m68k_in_mem(ctx, ch->entry, 2) || !(ch->stack = M68KEmu_HeapAlloc(ctx, ch->stacksize, 0)))
    {
        if (ctx->trace_calls || ctx->trace_end)
        {
            ULONG st = M68KEmu_GetD(cpu, 1), t2;
            bug("[m68kemu] CreateNewProc: no 68k entry (%08lx), not started; tags:\n", (unsigned long)ch->entry);
            while ((t2 = m68k_next_tag(ctx, &st)))
                bug("[m68kemu]   %08lx = %08lx\n", (unsigned long)m68k_read32(ctx, t2), (unsigned long)m68k_read32(ctx, t2 + 4));
        }
        FreeVec(ch);
        FreeVec(name);
        return 0;
    }
    ch->parent = FindTask(NULL);
    t[n].ti_Tag = NP_Entry;     t[n++].ti_Data = (IPTR)child_entry;
    t[n].ti_Tag = NP_UserData;  t[n++].ti_Data = (IPTR)ch;
    t[n].ti_Tag = NP_StackSize; t[n++].ti_Data = 65536;
    t[n].ti_Tag = TAG_DONE;
    /* counted before it runs: the parent may return first */
    ctx->num_children++;
    m68k_context_hold(ctx);
    p = CreateNewProc(t);
    FreeVec(name);
    if (!p)
    {
        ctx->num_children--;
        m68k_context_release(ctx);
        M68KEmu_HeapFree(ctx, ch->stack, ch->stacksize);
        FreeVec(ch);
        return 0;
    }
    return m68k_process_of(ctx, &p->pr_Task);
}

/* dos -492: Cli(): the program's CommandLineInterface, a BPTR */
IPTR m68k_dos_Cli(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mp = m68k_process_of(ctx, FindTask(NULL));
    (void)cpu;
    return mp ? m68k_read32(ctx, mp + M68K_PR_CLI) : 0;
}

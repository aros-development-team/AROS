/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library — more than one task in a 68k program: its processes,
    and the messages they and other programs send each other.

    Every task of the program runs its own CPU over the program's memory.
    The emulator's state is shared, so a CPU runs 68k code only while it
    holds the program's lock; it lets go in calls that wait, and now and
    then between instructions, so the tasks take turns as on AmigaOS.

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
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>

#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"
#include "m68kemu_offsets.h"

void *shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr);

/* ---------------------------------------------------------------------- */
/* the program's lock                                                     */

void m68k_gil_init(struct M68KEmuContext *ctx)
{
    InitSemaphore(&ctx->gil);
}

void m68k_gil_obtain(struct M68KEmuContext *ctx)
{
    ObtainSemaphore(&ctx->gil);
}

void m68k_gil_release(struct M68KEmuContext *ctx)
{
    if (ctx->gil.ss_Owner == FindTask(NULL))
        ReleaseSemaphore(&ctx->gil);
}

/* between instructions: let another task of the program run, unless the
   program has forbidden task switching */
void m68k_gil_yield(struct M68KEmuContext *ctx)
{
    if (ctx->forbid || ctx->gil.ss_QueueCount <= 0)   /* nobody waiting */
        return;
    ReleaseSemaphore(&ctx->gil);
    ObtainSemaphore(&ctx->gil);
}

/* the program's processes run in its memory: they end before it goes */
void m68k_wait_children(struct M68KEmuContext *ctx)
{
    for (int w = 0; ctx->num_children > 0 && w < 600; w++)
    {
        m68k_gil_release(ctx);
        Delay(5);
        m68k_gil_obtain(ctx);
    }
    if (ctx->num_children > 0)
        m68k_log(ctx, "%s: ended with %ld of its processes still running",
                 ctx->program_name[0] ? ctx->program_name : "68k program", (long)ctx->num_children);
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
#define LN_TYPE    8

static BOOL in_mem(struct M68KEmuContext *ctx, ULONG a, ULONG len)
{
    return a && a < ctx->mem_size && len <= ctx->mem_size - a;
}

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

    if (!in_mem(ctx, head, 8) || !(succ = m68k_read32(ctx, head)))
        return 0;
    m68k_write32(ctx, list, succ);
    m68k_write32(ctx, succ + 4, list);
    return head;
}

/* a port in the program's memory: one without a native port behind it */
BOOL m68k_port_is_68k(struct M68KEmuContext *ctx, ULONG mport)
{
    return in_mem(ctx, mport, 34) && !shadow_lookup(ctx, mport);
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

void m68k_deliver(struct M68KEmuContext *ctx, ULONG mport, ULONG mmsg, UBYTE type)
{
    struct MsgPort *port;

    if (!in_mem(ctx, mmsg, 20))
        return;
    ctx->mem[mmsg + LN_TYPE] = type;
    if ((port = (struct MsgPort *)shadow_lookup(ctx, mport)))
    {
        struct M68KCarrier *c = AllocVec(sizeof(*c), MEMF_PUBLIC | MEMF_CLEAR);
        if (!c)
            return;
        c->msg.mn_Node.ln_Name = (STRPTR)carrier_name;
        c->msg.mn_Length = sizeof(*c);
        c->mmsg = mmsg;
        c->type = type;
        PutMsg(port, &c->msg);
        return;
    }
    if (!in_mem(ctx, mport, 34))
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

    if (!in_mem(ctx, mmsg, 20))
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
    ULONG regs[15] = { 0 }, mproc;
    LONG rc;

    m68k_gil_obtain(ctx);
    ctx->num_children++;
    mproc = m68k_process_of(ctx, &me->pr_Task);
    regs[14] = ctx->m68k_sysbase;              /* A6 */
    (void)mproc;
    rc = (LONG)M68KEmu_RunTask(ctx, ch->entry, ch->stack + ch->stacksize, regs);
    if (ch->exitcode)
    {
        regs[0] = rc;
        regs[1] = ch->exitdata;
        M68KEmu_RunTask(ctx, ch->exitcode, ch->stack + ch->stacksize, regs);
    }
    M68KEmu_HeapFree(ctx, ch->stack, ch->stacksize);
    ctx->num_children--;
    m68k_gil_release(ctx);
    FreeVec(ch);
    return rc;
}

/* dos -498: CreateNewProc(D1=tags): NP_Entry is 68k code of the program */
IPTR m68k_dos_CreateNewProc(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG state = M68KEmu_GetD(cpu, 1), ti;
    struct M68KChild *ch;
    struct TagItem t[16];
    struct Process *p;
    char *name = NULL;
    int n = 0;

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
        case NP_StackSize:  if (data > ch->stacksize) ch->stacksize = (data + 15) & ~15; break;
        case NP_ExitCode:   ch->exitcode = data; break;
        case NP_ExitData:   ch->exitdata = data; break;
        case NP_Priority:   t[n].ti_Tag = tag; t[n++].ti_Data = (IPTR)(LONG)data; break;
        case NP_Name:
            if (in_mem(ctx, data, 1) && (name = AllocVec(strlen((char *)ctx->mem + data) + 1, MEMF_ANY)))
            {
                strcpy(name, (char *)ctx->mem + data);
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
            t[n].ti_Tag = tag; t[n++].ti_Data = data;
            break;
        }
    }
    if (!ch->entry || !in_mem(ctx, ch->entry, 2) || !(ch->stack = M68KEmu_HeapAlloc(ctx, ch->stacksize, 0)))
    {
        if (ctx->trace_calls || ctx->trace_end)
            bug("[m68kemu] CreateNewProc: no 68k entry (%08lx), not started\n", (unsigned long)ch->entry);
        FreeVec(ch);
        FreeVec(name);
        return 0;
    }
    ch->parent = FindTask(NULL);
    t[n].ti_Tag = NP_Entry;     t[n++].ti_Data = (IPTR)child_entry;
    t[n].ti_Tag = NP_UserData;  t[n++].ti_Data = (IPTR)ch;
    t[n].ti_Tag = NP_StackSize; t[n++].ti_Data = 65536;
    t[n].ti_Tag = TAG_DONE;
    p = CreateNewProc(t);
    FreeVec(name);
    if (!p)
    {
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

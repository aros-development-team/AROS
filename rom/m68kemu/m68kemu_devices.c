/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    m68kemu.library — device I/O for m68k programs. The program's IORequest
    (m68k layout) has a native twin; fields are copied in before each call
    and out after it. io_Device is an m68k device base whose BeginIO and
    AbortIO vectors (and timer.device's functions) are thunks.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <devices/timer.h>
#include <devices/audio.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/timer.h>
#define __CONSOLE_NOLIBBASE__
#include <proto/console.h>
#include <proto/alib.h>
#include <string.h>

#include "m68kemu_intern.h"
#include "m68kemu_thunks.h"

void *shadow_lookup(struct M68KEmuContext *ctx, ULONG m68k_addr);

/* m68k IORequest: Message 0..19 (mn_ReplyPort 14), io_Device 20, io_Unit 24,
   io_Command 28, io_Flags 30, io_Error 31; IOStdReq: io_Actual 32,
   io_Length 36, io_Data 40, io_Offset 44; timerequest: tv_secs 32, tv_micro 36 */
#define IO_REPLYPORT 14
#define IO_DEVICE    20
#define IO_UNIT      24
#define IO_COMMAND   28
#define IO_FLAGS     30
#define IO_ERROR     31
#define IO_ACTUAL    32
#define IO_LENGTH    36
#define IO_DATA      40
#define IO_OFFSET    44

enum { DEV_STD, DEV_TIMER, DEV_AUDIO, DEV_INPUT, DEV_CONSOLE };

/* m68k IOAudio after the IORequest: ioa_AllocKey 32, ioa_Data 34,
   ioa_Length 38, ioa_Period 42, ioa_Volume 44, ioa_Cycles 46 */
#define IOA_ALLOCKEY 32
#define IOA_DATA     34
#define IOA_LENGTH   38
#define IOA_PERIOD   42
#define IOA_VOLUME   44
#define IOA_CYCLES   46

/* devices whose request layout is translated; any other fails to open
   rather than handing a native device an m68k structure */
static const struct { const char *name; UBYTE kind; } known_devices[] = {
    { "timer.device", DEV_TIMER },
    { "audio.device", DEV_AUDIO },
    { "console.device", DEV_CONSOLE },
    { "input.device", DEV_INPUT },
    { "keyboard.device", DEV_STD },
    { "gameport.device", DEV_STD },
    { "trackdisk.device", DEV_STD },
    { "clipboard.device", DEV_STD },
    { NULL, 0 }
};

static struct M68KIoMap *io_find(struct M68KEmuContext *ctx, ULONG m68k)
{
    ULONG i;
    for (i = 0; i < M68KEMU_MAX_IOREQS; i++)
        if (ctx->ioreqs[i].m68k == m68k && ctx->ioreqs[i].native) return &ctx->ioreqs[i];
    return NULL;
}

ULONG m68k_ioreq_of(struct M68KEmuContext *ctx, APTR native)
{
    ULONG i;
    for (i = 0; i < M68KEMU_MAX_IOREQS; i++)
        if (ctx->ioreqs[i].native == native) return ctx->ioreqs[i].m68k;
    return 0;
}

static BOOL input_request(struct M68KEmuContext *ctx, struct M68KIoMap *m);
static BOOL paula_holder(struct M68KEmuContext *ctx, struct IORequest *io);

static void io_in(struct M68KEmuContext *ctx, struct M68KIoMap *m)
{
    struct IORequest *io = m->native;
    ULONG a = m->m68k;
    io->io_Command = m68k_read16(ctx, a + IO_COMMAND);
    io->io_Flags   = ctx->mem[a + IO_FLAGS];
    if (m->kind == DEV_TIMER)
    {
        struct timerequest *tr = (struct timerequest *)io;
        tr->tr_time.tv_secs  = m68k_read32(ctx, a + IO_ACTUAL);
        tr->tr_time.tv_micro = m68k_read32(ctx, a + IO_LENGTH);
    }
    else if (m->kind == DEV_AUDIO)
    {
        struct IOAudio *ia = (struct IOAudio *)io;
        ULONG data = m68k_read32(ctx, a + IOA_DATA);
        io->io_Unit      = (struct Unit *)(IPTR)m68k_read32(ctx, a + IO_UNIT);
        ia->ioa_AllocKey = (WORD)m68k_read16(ctx, a + IOA_ALLOCKEY);
        ia->ioa_Data     = data ? m68k_to_host(ctx, data) : NULL;
        ia->ioa_Length   = m68k_read32(ctx, a + IOA_LENGTH);
        ia->ioa_Period   = m68k_read16(ctx, a + IOA_PERIOD);
        ia->ioa_Volume   = m68k_read16(ctx, a + IOA_VOLUME);
        ia->ioa_Cycles   = m68k_read16(ctx, a + IOA_CYCLES);
    }
    else
    {
        struct IOStdReq *sr = (struct IOStdReq *)io;
        ULONG data = m68k_read32(ctx, a + IO_DATA);
        sr->io_Length = m68k_read32(ctx, a + IO_LENGTH);
        sr->io_Data   = data ? m68k_to_host(ctx, data) : NULL;
        sr->io_Offset = m68k_read32(ctx, a + IO_OFFSET);
    }
}

void m68k_ioreq_out(struct M68KEmuContext *ctx, APTR native)
{
    ULONG a = m68k_ioreq_of(ctx, native), i;
    struct IORequest *io = native;
    if (!a) return;
    for (i = 0; i < M68KEMU_MAX_IOREQS; i++)
        if (ctx->ioreqs[i].native == native) break;
    if (i == M68KEMU_MAX_IOREQS) return;
    ctx->ioreqs[i].pending = FALSE;
    ctx->mem[a + IO_ERROR] = (UBYTE)io->io_Error;
    ctx->mem[a + IO_FLAGS] = io->io_Flags;
    if (ctx->ioreqs[i].kind == DEV_TIMER)
    {
        struct timerequest *tr = (struct timerequest *)io;
        m68k_write32(ctx, a + IO_ACTUAL, tr->tr_time.tv_secs);
        m68k_write32(ctx, a + IO_LENGTH, tr->tr_time.tv_micro);
    }
    else if (ctx->ioreqs[i].kind == DEV_AUDIO)
    {
        struct IOAudio *ia = (struct IOAudio *)io;
        m68k_write32(ctx, a + IO_UNIT, (ULONG)(IPTR)io->io_Unit);
        m68k_write16(ctx, a + IOA_ALLOCKEY, (UWORD)ia->ioa_AllocKey);
        m68k_write32(ctx, a + IOA_LENGTH, ia->ioa_Length);
        if (io->io_Command == ADCMD_ALLOCATE && !io->io_Error && io->io_Unit)
            m68k_paula_attach(ctx, io);
        else if (io->io_Command == ADCMD_FREE && paula_holder(ctx, io))
            m68k_paula_free(ctx);
    }
    else
        m68k_write32(ctx, a + IO_ACTUAL, ((struct IOStdReq *)io)->io_Actual);
}

ULONG shadow_register(struct M68KEmuContext *ctx, ULONG m68k_addr, void *native, UWORD type);

/* the native side of a program's reply port: a port it made itself in m68k
   memory (CreatePort, AllocSignal + AddPort) gets a native twin on the same
   task and signal bit, registered as its shadow so WaitPort/GetMsg find it */
struct MsgPort *m68k_port_twin(struct M68KEmuContext *ctx, ULONG mport, BOOL *own)
{
    struct MsgPort *port;

    *own = FALSE;
    if (!mport)
    {
        *own = TRUE;
        return CreateMsgPort();
    }
    if ((port = (struct MsgPort *)shadow_lookup(ctx, mport)))
        return port;
    port = AllocVec(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
    if (!port)
        return NULL;
    port->mp_Node.ln_Type = NT_MSGPORT;
    port->mp_Flags = PA_SIGNAL;
    port->mp_SigBit = ctx->mem[mport + 15];
    port->mp_SigTask = FindTask(NULL);
    NEWLIST(&port->mp_MsgList);
    shadow_register(ctx, mport, port, 4);
    return port;
}

/* ── the m68k device base: BeginIO/AbortIO and timer.device functions ── */
static IPTR dev_BeginIO(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KIoMap *m = io_find(ctx, M68KEmu_GetA(cpu, 1));
    if (!m) return 0;
    io_in(ctx, m);
    if (input_request(ctx, m))
        return 0;
    m->pending = TRUE;
    BeginIO(m->native);
    if (m->native->io_Flags & IOF_QUICK) m68k_ioreq_out(ctx, m->native);
    return 0;
}

static IPTR dev_AbortIO(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KIoMap *m = io_find(ctx, M68KEmu_GetA(cpu, 1));
    /* only a request in flight: aborting an idle one is a no-op on AmigaOS */
    if (m && m->pending && m->native->io_Unit && !CheckIO(m->native)) AbortIO(m->native);
    return 0;
}

static struct Device *timer_base(struct M68KEmuContext *ctx)
{
    ULONG i;
    for (i = 0; i < M68KEMU_MAX_IOREQS; i++)
        if (ctx->ioreqs[i].native && ctx->ioreqs[i].kind == DEV_TIMER)
            return ctx->ioreqs[i].native->io_Device;
    return NULL;
}

static void tv_get(struct M68KEmuContext *ctx, ULONG a, struct timeval *tv)
{
    tv->tv_secs = m68k_read32(ctx, a);
    tv->tv_micro = m68k_read32(ctx, a + 4);
}

static void tv_put(struct M68KEmuContext *ctx, ULONG a, struct timeval *tv)
{
    m68k_write32(ctx, a, tv->tv_secs);
    m68k_write32(ctx, a + 4, tv->tv_micro);
}

static IPTR tm_AddTime(struct M68KEmuContext *ctx, void *cpu)
{
    struct Device *TimerBase = timer_base(ctx);
    struct timeval d, s;
    if (!TimerBase) return 0;
    tv_get(ctx, M68KEmu_GetA(cpu, 0), &d); tv_get(ctx, M68KEmu_GetA(cpu, 1), &s);
    AddTime(&d, &s);
    tv_put(ctx, M68KEmu_GetA(cpu, 0), &d);
    return 0;
}

static IPTR tm_SubTime(struct M68KEmuContext *ctx, void *cpu)
{
    struct Device *TimerBase = timer_base(ctx);
    struct timeval d, s;
    if (!TimerBase) return 0;
    tv_get(ctx, M68KEmu_GetA(cpu, 0), &d); tv_get(ctx, M68KEmu_GetA(cpu, 1), &s);
    SubTime(&d, &s);
    tv_put(ctx, M68KEmu_GetA(cpu, 0), &d);
    return 0;
}

static IPTR tm_CmpTime(struct M68KEmuContext *ctx, void *cpu)
{
    struct Device *TimerBase = timer_base(ctx);
    struct timeval d, s;
    if (!TimerBase) return 0;
    tv_get(ctx, M68KEmu_GetA(cpu, 0), &d); tv_get(ctx, M68KEmu_GetA(cpu, 1), &s);
    return (IPTR)(ULONG)CmpTime(&d, &s);
}

static IPTR tm_ReadEClock(struct M68KEmuContext *ctx, void *cpu)
{
    struct Device *TimerBase = timer_base(ctx);
    struct EClockVal ev;
    ULONG f, a = M68KEmu_GetA(cpu, 0);
    if (!TimerBase) return 0;
    f = ReadEClock(&ev);
    if (a) { m68k_write32(ctx, a, ev.ev_hi); m68k_write32(ctx, a + 4, ev.ev_lo); }
    return f;
}

static IPTR tm_GetSysTime(struct M68KEmuContext *ctx, void *cpu)
{
    struct Device *TimerBase = timer_base(ctx);
    struct timeval tv;
    if (!TimerBase) return 0;
    GetSysTime(&tv);
    tv_put(ctx, M68KEmu_GetA(cpu, 0), &tv);
    return 0;
}

static struct Device *console_base(struct M68KEmuContext *ctx)
{
    ULONG i;
    for (i = 0; i < M68KEMU_MAX_IOREQS; i++)
        if (ctx->ioreqs[i].native && ctx->ioreqs[i].kind == DEV_CONSOLE)
            return ctx->ioreqs[i].native->io_Device;
    return NULL;
}

/* m68k InputEvent: ie_Class 4, ie_SubClass 5, ie_Code 6, ie_Qualifier 8,
   ie_position 10 (dead-key bytes for RAWKEY) */
static IPTR con_RawKeyConvert(struct M68KEmuContext *ctx, void *cpu)
{
    struct Device *ConsoleDevice = console_base(ctx);
    struct InputEvent ie = { 0 };
    ULONG mie = M68KEmu_GetA(cpu, 0), mbuf = M68KEmu_GetA(cpu, 1);
    LONG len = (LONG)M68KEmu_GetD(cpu, 1);

    if (!ConsoleDevice || !m68k_in_mem(ctx, mie, 14) || len < 0 || !m68k_in_mem(ctx, mbuf, (ULONG)len))
        return (IPTR)-1;
    ie.ie_Class     = ctx->mem[mie + 4];
    ie.ie_SubClass  = ctx->mem[mie + 5];
    ie.ie_Code      = m68k_read16(ctx, mie + 6);
    ie.ie_Qualifier = m68k_read16(ctx, mie + 8);
    CopyMem(ctx->mem + mie + 10, &ie.ie_position, 4);
    /* an m68k KeyMap has a different layout: use the default one */
    return (IPTR)RawKeyConvert(&ie, (STRPTR)(ctx->mem + mbuf), len, NULL);
}

static const struct M68KThunkEntry dev_std_tab[] = {
    { 30, dev_BeginIO }, { 36, dev_AbortIO },
};
static const struct M68KThunkEntry dev_timer_tab[] = {
    { 30, dev_BeginIO }, { 36, dev_AbortIO },
    { 42, tm_AddTime }, { 48, tm_SubTime }, { 54, tm_CmpTime },
    { 60, tm_ReadEClock }, { 66, tm_GetSysTime },
};
static const struct M68KThunkEntry dev_console_tab[] = {
    { 30, dev_BeginIO }, { 36, dev_AbortIO }, { 48, con_RawKeyConvert },
};

/* exec -444: OpenDevice(A0=name, D0=unit, A1=ioRequest, D1=flags) -> D0 error */
IPTR m68k_OpenDevice(struct M68KEmuContext *ctx, void *cpu)
{
    ULONG mname = M68KEmu_GetA(cpu, 0), mio = M68KEmu_GetA(cpu, 1);
    const char *name = mname ? (const char *)m68k_to_host(ctx, mname) : NULL;
    struct MsgPort *port;
    struct IORequest *io;
    ULONG i, kind, size, devbase;
    BYTE err;

    if (!name || !mio) return (IPTR)(LONG)IOERR_OPENFAIL;
    for (i = 0; known_devices[i].name && strcmp(known_devices[i].name, name); i++)
        ;
    if (!known_devices[i].name)
    {
        m68k_log(ctx, "%s: %s is not supported for 68k programs yet",
                 ctx->program_name[0] ? ctx->program_name : "68k program", name);
        ctx->mem[mio + IO_ERROR] = (UBYTE)IOERR_OPENFAIL;
        return (IPTR)(LONG)IOERR_OPENFAIL;
    }
    kind = known_devices[i].kind;
    size = kind == DEV_TIMER ? sizeof(struct timerequest)
         : kind == DEV_AUDIO ? sizeof(struct IOAudio) : sizeof(struct IOStdReq);
    for (i = 0; i < M68KEMU_MAX_IOREQS && ctx->ioreqs[i].native; i++)
        ;
    if (i == M68KEMU_MAX_IOREQS) return (IPTR)(LONG)IOERR_OPENFAIL;

    {
        BOOL own;
        port = m68k_port_twin(ctx, m68k_read32(ctx, mio + IO_REPLYPORT), &own);
        if (!port) return (IPTR)(LONG)IOERR_OPENFAIL;
        ctx->ioreqs[i].ownport = own ? port : NULL;
    }
    io = AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
    if (!io) return (IPTR)(LONG)IOERR_OPENFAIL;
    io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    io->io_Message.mn_ReplyPort = port;
    io->io_Message.mn_Length = size;
    ctx->ioreqs[i].m68k = mio;
    ctx->ioreqs[i].native = io;
    ctx->ioreqs[i].kind = kind;
    io_in(ctx, &ctx->ioreqs[i]);

    err = OpenDevice((CONST_STRPTR)name, M68KEmu_GetD(cpu, 0), io, M68KEmu_GetD(cpu, 1));
    if (err)
    {
        ctx->ioreqs[i].native = NULL;
        ctx->ioreqs[i].m68k = 0;
        FreeVec(io);
        if (ctx->ioreqs[i].ownport) DeleteMsgPort(ctx->ioreqs[i].ownport);
        ctx->ioreqs[i].ownport = NULL;
        ctx->mem[mio + IO_ERROR] = (UBYTE)err;
        return (IPTR)(LONG)err;
    }
    /* the program calls the device through io_Device: an m68k base with thunks */
    devbase = M68KEmu_FindLibBase(ctx, name);
    if (!devbase)
        devbase = M68KEmu_SetupLibBase(ctx, ctx->num_libs, name, 16,
                                       kind == DEV_TIMER ? dev_timer_tab :
                                       kind == DEV_CONSOLE ? dev_console_tab : dev_std_tab,
                                       kind == DEV_TIMER ? 7 : kind == DEV_CONSOLE ? 3 : 2, NULL, 0);
    m68k_write32(ctx, mio + IO_DEVICE, devbase);
    m68k_write32(ctx, mio + IO_UNIT, kind == DEV_AUDIO ? (ULONG)(IPTR)io->io_Unit : m68k_bptr_out(ctx, (BPTR)io->io_Unit));
    if (kind == DEV_AUDIO && io->io_Unit)
        m68k_paula_attach(ctx, io);
    ctx->mem[mio + IO_ERROR] = 0;
    return 0;
}

/* ── The audio registers: a program that allocated its channels through
   audio.device and then writes AUD0-3 and DMACON itself plays through
   audio.device with its own allocation, a CMD_WRITE of the sample when
   DMA starts and one of the repeat part the program sets after that ── */
#define PAULA_DMACON 0x096
#define PAULA_AUD0   0x0A0

struct M68KPaulaChan
{
    ULONG lc;
    UWORD len, per, vol;
    BOOL dma;
    struct IOAudio *one, *rep, *ctl;
};

struct M68KPaula
{
    BOOL trace;
    UBYTE *ctx_mem;
    struct MsgPort *port;
    struct Device *device;
    struct IORequest *holder;     /* the request that allocated the channels */
    UWORD key;
    ULONG mask;
    struct M68KPaulaChan ch[4];
};

static BOOL paula_holder(struct M68KEmuContext *ctx, struct IORequest *io)
{
    return ctx->paula && ctx->paula->holder == io;
}

static void paula_reclaim(struct M68KPaula *p, struct IOAudio *io)
{
    if (!io->ioa_Request.io_Message.mn_Node.ln_Type)
        return;                                   /* never sent */
    if (!CheckIO((struct IORequest *)io))
        AbortIO((struct IORequest *)io);
    WaitIO((struct IORequest *)io);
    if (p->trace)
        bug("[m68kemu] paula: unit %lu cmd %u done, error %d\n", (unsigned long)(IPTR)io->ioa_Request.io_Unit,
            io->ioa_Request.io_Command, io->ioa_Request.io_Error);
    io->ioa_Request.io_Message.mn_Node.ln_Type = 0;
}

static struct IOAudio *paula_request(struct M68KPaula *p, int ch)
{
    struct IOAudio *io = AllocMem(sizeof(*io), MEMF_PUBLIC | MEMF_CLEAR);
    if (!io) return NULL;
    io->ioa_Request.io_Message.mn_ReplyPort = p->port;
    io->ioa_Request.io_Message.mn_Length = sizeof(*io);
    io->ioa_Request.io_Device = p->device;
    io->ioa_Request.io_Unit = (struct Unit *)(IPTR)(1 << ch);
    io->ioa_AllocKey = p->key;
    return io;
}

void m68k_paula_attach(struct M68KEmuContext *ctx, struct IORequest *io)
{
    struct IOAudio *ia = (struct IOAudio *)io;
    struct M68KPaula *p = ctx->paula;

    if (!p)
    {
        if (!(p = AllocMem(sizeof(*p), MEMF_PUBLIC | MEMF_CLEAR)))
            return;
        if (!(p->port = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR)))
        {
            FreeMem(p, sizeof(*p));
            return;
        }
        p->port->mp_Node.ln_Type = NT_MSGPORT;
        p->port->mp_Flags = PA_IGNORE;
        NEWLIST(&p->port->mp_MsgList);
        ctx->paula = p;
    }
    p->ctx_mem = ctx->mem;
    p->trace = ctx->trace_calls;
    p->device = io->io_Device;
    p->holder = io;
    if (p->trace)
        bug("[m68kemu] paula: audio.device channels %lu key %u\n", (unsigned long)(IPTR)io->io_Unit, ia->ioa_AllocKey);
    p->key = ia->ioa_AllocKey;
    p->mask = (ULONG)(IPTR)io->io_Unit & 15;
    for (int ch = 0; ch < 4; ch++)
    {
        struct M68KPaulaChan *c = &p->ch[ch];
        if (!c->one) c->one = paula_request(p, ch);
        if (!c->rep) c->rep = paula_request(p, ch);
        if (!c->ctl) c->ctl = paula_request(p, ch);
        if (c->one) { c->one->ioa_Request.io_Device = p->device; c->one->ioa_AllocKey = p->key; }
        if (c->rep) { c->rep->ioa_Request.io_Device = p->device; c->rep->ioa_AllocKey = p->key; }
        if (c->ctl) { c->ctl->ioa_Request.io_Device = p->device; c->ctl->ioa_AllocKey = p->key; }
    }
}

static void paula_send(struct M68KPaula *p, struct M68KPaulaChan *c, struct IOAudio *io, UWORD cycles)
{
    paula_reclaim(p, io);
    io->ioa_Request.io_Command = CMD_WRITE;
    io->ioa_Request.io_Flags = ADIOF_PERVOL;
    io->ioa_Data = (UBYTE *)(p->ctx_mem + c->lc);
    io->ioa_Length = (c->len ? c->len : 0x10000) * 2;
    io->ioa_Period = c->per;
    io->ioa_Volume = c->vol;
    io->ioa_Cycles = cycles;
    io->ioa_Request.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    if (p->trace)
        bug("[m68kemu] paula: unit %lu write %08lx len %lu per %u vol %u cycles %u\n",
            (unsigned long)(IPTR)io->ioa_Request.io_Unit, (unsigned long)c->lc, (unsigned long)io->ioa_Length,
            io->ioa_Period, io->ioa_Volume, cycles);
    SendIO((struct IORequest *)io);
}

static void paula_control(struct M68KPaula *p, struct M68KPaulaChan *c, UWORD cmd)
{
    struct IOAudio *io = c->ctl;
    if (!io) return;
    paula_reclaim(p, io);
    io->ioa_Request.io_Command = cmd;
    io->ioa_Request.io_Flags = ADIOF_PERVOL;
    io->ioa_Period = c->per;
    io->ioa_Volume = c->vol;
    io->ioa_Request.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    DoIO((struct IORequest *)io);
    if (p->trace)
        bug("[m68kemu] paula: unit %lu cmd %u per %u vol %u -> error %d\n", (unsigned long)(IPTR)io->ioa_Request.io_Unit,
            cmd, io->ioa_Period, io->ioa_Volume, io->ioa_Request.io_Error);
    io->ioa_Request.io_Message.mn_Node.ln_Type = 0;
}

static void paula_stop(struct M68KPaula *p, struct M68KPaulaChan *c)
{
    if (c->one) paula_reclaim(p, c->one);
    if (c->rep) paula_reclaim(p, c->rep);
    paula_control(p, c, CMD_FLUSH);
}

UWORD m68k_paula_dmacon(struct M68KEmuContext *ctx)
{
    UWORD v = 0;
    if (!ctx->paula) return 0;
    for (int ch = 0; ch < 4; ch++)
        if (ctx->paula->ch[ch].dma) v |= 1 << ch;
    return v;
}

void m68k_paula_write(struct M68KEmuContext *ctx, UWORD reg, UWORD val)
{
    struct M68KPaula *p = ctx->paula;
    int ch;

    if (!p || !p->device)
        return;
    p->trace = ctx->trace_calls;
    if (reg == PAULA_DMACON)
    {
        for (ch = 0; ch < 4; ch++)
        {
            struct M68KPaulaChan *c = &p->ch[ch];
            if (!(val & (1 << ch)) || !(p->mask & (1 << ch)) || !c->one || !c->rep)
                continue;
            if (val & 0x8000)
            {
                if (c->dma) continue;
                c->dma = TRUE;
                if (!m68k_in_mem(ctx, c->lc, 2)) continue;
                paula_reclaim(p, c->rep);
                paula_send(p, c, c->one, 1);
            }
            else if (c->dma)
            {
                c->dma = FALSE;
                paula_stop(p, c);
            }
        }
        return;
    }
    if (reg < PAULA_AUD0 || reg >= PAULA_AUD0 + 4 * 16)
        return;
    ch = (reg - PAULA_AUD0) >> 4;
    {
        struct M68KPaulaChan *c = &p->ch[ch];
        switch (reg & 15)
        {
        case 0:  c->lc = (c->lc & 0xFFFF) | ((ULONG)(val & 0x1F) << 16); break;
        case 2:  c->lc = (c->lc & 0xFFFF0000) | (val & 0xFFFE); break;
        case 4:
            c->len = val;
            /* the repeat part, set once the sample started: plays after it */
            if (c->dma && c->rep && m68k_in_mem(ctx, c->lc, 2))
                paula_send(p, c, c->rep, 0);
            break;
        case 6:
            if (c->per == val) break;
            c->per = val;
            if (c->dma) paula_control(p, c, ADCMD_PERVOL);
            break;
        case 8:
            val &= 0x7F;
            if (val > 64) val = 64;
            if (c->vol == val) break;
            c->vol = val;
            if (c->dma) paula_control(p, c, ADCMD_PERVOL);
            break;
        default: break;
        }
    }
}

void m68k_paula_free(struct M68KEmuContext *ctx)
{
    struct M68KPaula *p = ctx->paula;
    struct Message *msg;

    if (!p) return;
    for (int ch = 0; ch < 4; ch++)
    {
        struct M68KPaulaChan *c = &p->ch[ch];
        if (c->dma && p->device) { c->dma = FALSE; paula_stop(p, c); }
        if (c->one) FreeMem(c->one, sizeof(struct IOAudio));
        if (c->rep) FreeMem(c->rep, sizeof(struct IOAudio));
        if (c->ctl) FreeMem(c->ctl, sizeof(struct IOAudio));
    }
    while ((msg = GetMsg(p->port)))
        ;
    FreeMem(p->port, sizeof(struct MsgPort));
    FreeMem(p, sizeof(*p));
    ctx->paula = NULL;
}

/* exec -450: CloseDevice(A1=ioRequest) */
IPTR m68k_CloseDevice(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KIoMap *m = io_find(ctx, M68KEmu_GetA(cpu, 1));
    if (!m) return 0;
    if (m->kind == DEV_AUDIO && paula_holder(ctx, m->native))
        m68k_paula_free(ctx);
    CloseDevice(m->native);
    FreeVec(m->native);
    if (m->ownport) DeleteMsgPort(m->ownport);
    m->ownport = NULL;
    m->native = NULL;
    m->m68k = 0;
    return 0;
}

/* input.device requests carrying 68k structures. A handler (IND_ADDHANDLER)
   is 68k code input.device would call as native code: it is accepted and not
   installed, so the program's handler sees no events. Events written with
   IND_WRITEEVENT are made native: m68k struct InputEvent: ie_NextEvent 0,
   ie_Class 4, ie_SubClass 5, ie_Code 6, ie_Qualifier 8, ie_X/ie_Y 10/12,
   ie_TimeStamp 14 */
static struct InputEvent input_events[16];

static BOOL input_request(struct M68KEmuContext *ctx, struct M68KIoMap *m)
{
    struct IOStdReq *sr = (struct IOStdReq *)m->native;
    ULONG a = m->m68k, e;
    int n = 0;

    if (m->kind != DEV_INPUT)
        return FALSE;
    switch (m68k_read16(ctx, a + IO_COMMAND))
    {
    case IND_ADDHANDLER:
    case IND_REMHANDLER:
        if (ctx->trace_calls || ctx->trace_end)
            bug("[m68kemu] input.device: a 68k input handler is not installed\n");
        ctx->mem[a + IO_ERROR] = 0;
        return TRUE;
    case IND_WRITEEVENT:
        for (e = m68k_read32(ctx, a + IO_DATA); m68k_in_mem(ctx, e, 22) && n < 16; e = m68k_read32(ctx, e), n++)
        {
            struct InputEvent *ie = &input_events[n];
            memset(ie, 0, sizeof(*ie));
            ie->ie_Class = ctx->mem[e + 4];
            ie->ie_SubClass = ctx->mem[e + 5];
            ie->ie_Code = m68k_read16(ctx, e + 6);
            ie->ie_Qualifier = m68k_read16(ctx, e + 8);
            ie->ie_X = (WORD)m68k_read16(ctx, e + 10);
            ie->ie_Y = (WORD)m68k_read16(ctx, e + 12);
            if (n)
                input_events[n - 1].ie_NextEvent = ie;
        }
        sr->io_Data = n ? &input_events[0] : NULL;
        sr->io_Length = sizeof(struct InputEvent);
        return FALSE;
    }
    return FALSE;
}

/* exec -456: DoIO(A1) -> D0 error */
IPTR m68k_DoIO(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KIoMap *m = io_find(ctx, M68KEmu_GetA(cpu, 1));
    BYTE err;
    if (!m) return (IPTR)(LONG)IOERR_NOCMD;
    io_in(ctx, m);
    if (input_request(ctx, m))
        return 0;
    m68k_gil_release(ctx);
    err = DoIO(m->native);
    m68k_gil_obtain(ctx);
    m68k_ioreq_out(ctx, m->native);
    return (IPTR)(LONG)err;
}

/* exec -462: SendIO(A1) */
IPTR m68k_SendIO(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KIoMap *m = io_find(ctx, M68KEmu_GetA(cpu, 1));
    if (!m) return 0;
    io_in(ctx, m);
    if (input_request(ctx, m))
        return 0;
    m->pending = TRUE;
    SendIO(m->native);
    return 0;
}

/* exec -468: CheckIO(A1) -> D0 ioRequest or NULL */
IPTR m68k_CheckIO(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KIoMap *m = io_find(ctx, M68KEmu_GetA(cpu, 1));
    if (!m) return 0;
    if (!CheckIO(m->native)) return 0;
    m68k_ioreq_out(ctx, m->native);
    return m->m68k;
}

/* exec -474: WaitIO(A1) -> D0 error */
IPTR m68k_WaitIO(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KIoMap *m = io_find(ctx, M68KEmu_GetA(cpu, 1));
    BYTE err;
    if (!m) return (IPTR)(LONG)IOERR_NOCMD;
    m68k_gil_release(ctx);
    err = WaitIO(m->native);
    m68k_gil_obtain(ctx);
    m68k_ioreq_out(ctx, m->native);
    return (IPTR)(LONG)err;
}

/* exec -480: AbortIO(A1) */
IPTR m68k_AbortIO(struct M68KEmuContext *ctx, void *cpu)
{
    struct M68KIoMap *m = io_find(ctx, M68KEmu_GetA(cpu, 1));
    /* only a request in flight: aborting an idle one is a no-op on AmigaOS */
    if (m && m->pending && m->native->io_Unit && !CheckIO(m->native)) AbortIO(m->native);
    return 0;
}

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Palette notifications never render, take the compositor lock, or retain
    a bitmap. One cancelable VBlank request coalesces each burst of changes.
*/
#include <exec/memory.h>
#include <exec/tasks.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <string.h>
#include "gfx_intern.h"
#include "gfx_display.h"
#include "gfx_compositor.h"

#undef csd

/* The private base instance is only valid for the base compositor hierarchy. */
static struct HIDDCompositorData *palette_data(OOP_Class *base, OOP_Object *o)
{
    OOP_Class *cl;

    if (!base || !o)
        return NULL;
    for (cl = OOP_OCLASS(o); cl; cl = cl->superclass)
        if (cl == base)
            return OOP_INST_DATA(base, o);
    return NULL;
}

BOOL Compositor__Internal__CanHandleSoftware(OOP_Class *cl, OOP_Object *o)
{
    struct HIDDCompositorData *data = palette_data(cl, o);
    return data && data->can_compose;
}

void GfxPaletteInit(struct HIDDCompositorData *data)
{
    InitSemaphore(&data->palette.control);
    InitSemaphore(&data->palette.notify);
}

/* No allocation is needed to join a worker, including constructor failure. */
static ULONG palette_reply_port(struct MsgPort *port, struct Message *reply)
{
    ULONG saved = SetSignal(0, SIGF_SINGLE) & SIGF_SINGLE;

    memset(port, 0, sizeof(*port));
    port->mp_Node.ln_Type = NT_MSGPORT;
    port->mp_Flags = PA_SIGNAL;
    port->mp_SigBit = SIGB_SINGLE;
    port->mp_SigTask = FindTask(NULL);
    NEWLIST(&port->mp_MsgList);
    memset(reply, 0, sizeof(*reply));
    reply->mn_Node.ln_Type = NT_MESSAGE;
    reply->mn_Length = sizeof(*reply);
    reply->mn_ReplyPort = port;
    return saved;
}

static void palette_wait_reply(struct MsgPort *port, struct Task *worker)
{
    /* Consume the sender's signal before any semaphore wait can use SINGLE. */
    Wait(SIGF_SINGLE);
    WaitPort(port);
    GetMsg(port);
    if (worker)
    {
        /*
         * A reply alone does not mean a remote CPU has stopped executing.
         * The worker forbids scheduling before replying and parks in Wait(0).
         * Wait holds the task spinlock through its context save; RemTask then
         * takes that lock before reclaiming the already parked task.
         */
        while (((volatile struct Task *)worker)->tc_State != TS_WAIT)
            ;
        RemTask(worker);
    }
}

static void palette_park(struct Message *reply)
{
    /* There are no compositor or module-data accesses after the reply. */
    Forbid();
    ReplyMsg(reply);
    for (;;)
        Wait(0);
}

static void palette_task(struct HIDDCompositorData *data)
{
    struct GfxPaletteWorker *state = &data->palette;
    struct MsgPort *port;
    struct timerequest *timer = NULL;
    struct Message *reply;
    LONG bit = -1;
    BOOL opened = FALSE, pending = FALSE;

    /* NewCreateTaskA's parent-side hooks may still block after the child is
     * runnable on SMP. Do not reply into that call's reserved signal. */
    Wait(SIGF_SINGLE);
    port = CreateMsgPort();

    if (port)
    {
        timer = (struct timerequest *)CreateIORequest(port, sizeof(*timer));
        if (timer && !OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)timer, 0))
        {
            opened = TRUE;
            bit = AllocSignal(-1);
        }
    }
    if (bit >= 0)
    {
        ULONG control = 1UL << bit;
        ULONG timerbit = 1UL << port->mp_SigBit;

        ObtainSemaphore(&state->notify);
        state->task = FindTask(NULL);
        state->signal = control;
        state->ready = TRUE;
        ReplyMsg(&state->reply);
        /* The parent's stack reply port lives until this lock is released. */
        ReleaseSemaphore(&state->notify);

        for (;;)
        {
            BOOL dirty, stopping;
            ULONG signals;

            ObtainSemaphore(&state->notify);
            stopping = state->stopping;
            dirty = state->dirty;
            ReleaseSemaphore(&state->notify);
            if (stopping)
                break;
            if (dirty && !pending)
            {
                timer->tr_node.io_Command = TR_ADDREQUEST;
                timer->tr_time.tv_secs = 0;
                timer->tr_time.tv_micro = 1;
                SendIO((struct IORequest *)timer);
                pending = TRUE;
            }
            signals = Wait(control | timerbit);
            ObtainSemaphore(&state->notify);
            stopping = state->stopping;
            ReleaseSemaphore(&state->notify);
            if (stopping)
                break;
            if (pending && (signals & timerbit) && CheckIO((struct IORequest *)timer))
            {
                WaitIO((struct IORequest *)timer);
                pending = FALSE;
                ObtainSemaphore(&state->notify);
                dirty = state->dirty;
                state->dirty = FALSE;
                ReleaseSemaphore(&state->notify);
                if (dirty)
                    Compositor__Internal__FlushPalette(data);
            }
            /* Further pen changes never cancel or postpone an armed timer. */
        }
    }

    if (pending)
    {
        AbortIO((struct IORequest *)timer);
        WaitIO((struct IORequest *)timer);
    }
    if (opened)
        CloseDevice((struct IORequest *)timer);
    if (timer)
        DeleteIORequest((struct IORequest *)timer);
    if (port)
        DeleteMsgPort(port);
    if (bit >= 0)
        FreeSignal(bit);
    ObtainSemaphore(&state->notify);
    state->ready = FALSE;
    reply = &state->reply;
    ReleaseSemaphore(&state->notify);
    palette_park(reply);
}

BOOL GfxPaletteEnsure(struct HIDDCompositorData *data)
{
    struct GfxPaletteWorker *state = &data->palette;
    struct MsgPort port;
    struct Task *worker;
    ULONG saved;
    BOOL ready;
    struct TagItem tags[] =
    {
        { TASKTAG_NAME,      (IPTR)"gfx palette refresh" },
        { TASKTAG_PRI,       0 },
        { TASKTAG_STACKSIZE, 65536 },
        { TASKTAG_PC,        (IPTR)palette_task },
        { TASKTAG_ARG1,      (IPTR)data },
        { TAG_DONE,          0 }
    };

    ObtainSemaphore(&state->control);
    if (state->stopping)
    {
        ReleaseSemaphore(&state->control);
        return FALSE;
    }
    if (state->ready)
    {
        ReleaseSemaphore(&state->control);
        return TRUE;
    }
    saved = palette_reply_port(&port, &state->reply);
    worker = NewCreateTaskA(tags);
    if (!worker)
    {
        if (saved)
            Signal(FindTask(NULL), saved);
        ReleaseSemaphore(&state->control);
        return FALSE;
    }
    Signal(worker, SIGF_SINGLE);
    Wait(SIGF_SINGLE);
    WaitPort(&port);
    GetMsg(&port);
    ObtainSemaphore(&state->notify);
    ready = state->ready;
    if (ready && state->dirty)
        Signal(worker, state->signal);
    ReleaseSemaphore(&state->notify);
    if (!ready)
    {
        /* Initialization failure replied only after releasing all resources. */
        while (((volatile struct Task *)worker)->tc_State != TS_WAIT)
            ;
        RemTask(worker);
    }
    SetSignal(0, SIGF_SINGLE);
    if (saved)
        Signal(FindTask(NULL), saved);
    ReleaseSemaphore(&state->control);
    return ready;
}

void GfxPaletteQueue(struct HIDDCompositorData *data)
{
    struct GfxPaletteWorker *state = &data->palette;

    ObtainSemaphore(&state->notify);
    if (!state->stopping)
    {
        state->dirty = TRUE;
        if (state->ready)
            Signal(state->task, state->signal);
    }
    ReleaseSemaphore(&state->notify);
}

void GfxPaletteStop(struct HIDDCompositorData *data)
{
    struct GfxPaletteWorker *state = &data->palette;
    struct MsgPort port;
    struct Task *worker;
    ULONG saved = 0;
    BOOL join;

    ObtainSemaphore(&state->control);
    ObtainSemaphore(&state->notify);
    state->stopping = TRUE;
    worker = state->task;
    join = worker && state->ready;
    if (join)
    {
        saved = palette_reply_port(&port, &state->reply);
        Signal(worker, state->signal);
    }
    ReleaseSemaphore(&state->notify);
    if (join)
        palette_wait_reply(&port, worker);
    ObtainSemaphore(&state->notify);
    state->task = NULL;
    state->signal = 0;
    state->ready = FALSE;
    ReleaseSemaphore(&state->notify);
    ReleaseSemaphore(&state->control);
    if (join)
    {
        SetSignal(0, SIGF_SINGLE);
        if (saved)
            Signal(FindTask(NULL), saved);
    }
}

void Compositor__Internal__PaletteChanged(struct class_static_data *csd, OOP_Object *display)
{
    struct HIDDDisplayData *owner;
    struct HIDDCompositorData *data;

    if (!display)
        return;
    owner = OOP_INST_DATA(csd->displayclass, display);
    data = palette_data(csd->compositorclass, owner->compositor);
    if (data)
        GfxPaletteQueue(data);
}

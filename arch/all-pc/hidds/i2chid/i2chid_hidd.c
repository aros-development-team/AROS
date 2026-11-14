/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#define __OOP_NOLIBBASE__
#define __OOP_NOATTRBASES__
#define __OOP_NOMETHODBASES__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <aros/symbolsets.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <hidd/hidd.h>
#include <hidd/input.h>
#include "i2chid_hidd.h"
#include "i2chid_driver.h"
#include LC_LIBDEFS_FILE

#define OOPBase (b->oop)

/* Mouse consumers expect callbacks with interrupts disabled. */
static int pointer_event(void *context, int x, int y, unsigned type, unsigned button)
{
    struct i2chid_mouse *d = context;
    struct i2chid_base *b = d->base;
    struct pHidd_Mouse_Event event;
    struct pHW_Input_PushEvent msg;

    event.x = x;
    event.y = y;
    event.button = button ? button : vHidd_Mouse_NoButton;
    event.type = type == IH_PRESS     ? vHidd_Mouse_Press
                 : type == IH_RELEASE ? vHidd_Mouse_Release
                                      : vHidd_Mouse_Motion;
    msg.mID = b->inputBase + moHW_Input_PushEvent;
    msg.driver = d->self;
    msg.iedata = &event;
    Disable();
    OOP_DoMethod(b->mousehw, (OOP_Msg)&msg);
    Enable();
    return TRUE;
}

static int pointer_ready(void *context)
{
    struct i2chid_mouse *d = context;
    ULONG signals;

    d->base->status = "device ready";
    d->ready = TRUE;
    Signal(d->waiter, d->signal);
    /* Do not deliver input until HW_AddDriver has finished registration. */
    signals = Wait(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
    if (signals & SIGBREAKF_CTRL_C)
    {
        SetSignal(SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
        return FALSE;
    }
    return TRUE;
}

static void worker(void)
{
    struct Process *process = (struct Process *)FindTask(NULL);
    struct i2chid_mouse *d = process->pr_Task.tc_UserData;
    struct ih_sink sink = {d, pointer_event, pointer_ready};
    d->base->probes++;
    d->base->status = "probing";
    ih_driver_run(NULL, TRUE, FALSE, &sink, NULL);
    d->base->status = d->ready ? "input stopped" : "device unavailable";
    if (!d->ready)
    {
        Signal(d->waiter, d->signal);
    }
    /* Keep the task valid until Dispose requests its final exit. */
    while (!d->stopping)
    {
        Wait(SIGBREAKF_CTRL_C);
    }
}

static void worker_exit(IPTR result, IPTR context)
{
    struct i2chid_mouse *d = (struct i2chid_mouse *)context;
    struct Task *waiter = d->waiter;
    ULONG signal = d->signal;
    (void)result;
    /* Last instance access; the resident module remains loaded. */
    Signal(waiter, signal);
}

static void stop_worker(struct i2chid_mouse *d)
{
    /* SIGB_SINGLE is reserved for synchronous operations in the calling task. */
    d->waiter = FindTask(NULL);
    d->signal = SIGF_SINGLE;
    SetSignal(0, SIGF_SINGLE);
    d->stopping = TRUE;
    Signal(d->worker, SIGBREAKF_CTRL_C);
    Wait(SIGF_SINGLE);
    d->worker = NULL;
}

OOP_Object *I2CHID__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct i2chid_base *b = (struct i2chid_base *)cl->UserData;
    struct i2chid_mouse *d;
    struct Process *process;
    BYTE ready_signal;

    if (b->mouse)
    {
        return NULL;
    }
    b->status = "allocating ready signal";
    ready_signal = AllocSignal(-1);
    if (ready_signal == -1)
    {
        return NULL;
    }
    b->status = "creating input object";
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!o)
    {
        FreeSignal(ready_signal);
        return NULL;
    }
    d = OOP_INST_DATA(cl, o);
    d->base = b;
    d->self = o;
    d->waiter = FindTask(NULL);
    d->signal = 1UL << ready_signal;
    /* Resident initialization must not open or inherit shell streams.
     * The worker runs silently with explicit null handles. */
    b->status = "creating worker";
    process = CreateNewProcTags(NP_Entry, (IPTR)worker, NP_Name, (IPTR) "I2C HID touchpad",
                                NP_StackSize, 65536, NP_UserData, (IPTR)d, NP_ExitCode,
                                (IPTR)worker_exit, NP_ExitData, (IPTR)d, NP_Input, 0, NP_Output, 0,
                                NP_Error, 0, NP_CloseInput, FALSE, NP_CloseOutput, FALSE,
                                NP_CloseError, FALSE, NP_CurrentDir, 0, NP_HomeDir, 0, TAG_DONE);
    if (process)
    {
        d->worker = &process->pr_Task;
        Wait(d->signal);
        if (!d->ready)
        {
            stop_worker(d);
        }
    }
    FreeSignal(ready_signal);
    if (!d->worker)
    {
        OOP_MethodID dispose = msg->mID - moRoot_New + moRoot_Dispose;
        OOP_DoSuperMethod(cl, o, &dispose);
        return NULL;
    }
    b->mouse = o;
    return o;
}

void I2CHID__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct i2chid_base *b = (struct i2chid_base *)cl->UserData;
    struct i2chid_mouse *d = OOP_INST_DATA(cl, o);

    if (d->worker)
    {
        stop_worker(d);
    }
    b->mouse = NULL;
    b->registration = NULL;
    OOP_DoSuperMethod(cl, o, msg);
}

void I2CHID__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct i2chid_base *b = (struct i2chid_base *)cl->UserData;
    if (msg->attrID == b->mouseAB + aoHidd_Mouse_RelativeCoords)
    {
        *msg->storage = TRUE;
    }
    else if (msg->attrID == b->mouseAB + aoHidd_Mouse_Extended)
    {
        *msg->storage = FALSE;
    }
    else
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

static int init_attrs(struct i2chid_base *b)
{
    b->mouseAB = OOP_ObtainAttrBase((CONST_STRPTR)IID_Hidd_Mouse);
    b->hwBase = OOP_GetMethodID((CONST_STRPTR)IID_HW, 0);
    b->inputBase = OOP_GetMethodID((CONST_STRPTR)IID_HW_Input, 0);
    return b->mouseAB != 0;
}

static int init_driver(struct i2chid_base *b)
{
    struct pHW_AddDriver msg;
    b->status = "finding mouse subsystem";
    b->mousehw = OOP_NewObject(NULL, (CONST_STRPTR)CLID_HW_Mouse, NULL);
    if (b->mousehw)
    {
        b->status = "registering input driver";
        msg.mID = b->hwBase + moHW_AddDriver;
        msg.driverClass = b->mouseclass;
        msg.tags = NULL;
        b->registration = (OOP_Object *)OOP_DoMethod(b->mousehw, (OOP_Msg)&msg);
        if (b->registration)
        {
            struct i2chid_mouse *d = OOP_INST_DATA(b->mouseclass, b->mouse);
            Signal(d->worker, SIGBREAKF_CTRL_D);
        }
    }
    /* Stay resident even after a failed probe: a DOS exit hook may still return. */
    return TRUE;
}

ADD2INITLIB(init_attrs, 0);
ADD2INITLIB(init_driver, 10);

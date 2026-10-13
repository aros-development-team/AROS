/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: consumer registration and event delivery (callback, signal ring, message port)
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/nodes.h>
#include <proto/exec.h>
#include <hidd/controller.h>

#include "controller_intern.h"

#define SysBase ((struct ExecBase *)(CSD(cl)->cs_SysBase))

static inline BOOL ctrl_IsMotion(UWORD type)
{
    return type == vHidd_Controller_AxisMotion || type == vHidd_Controller_TouchMotion ||
           type == vHidd_Controller_SensorUpdate;
}

static void ctrl_RingPush(struct ControllerConsumer *con, const struct pHidd_Controller_Event *ev)
{
    /* called with interrupts disabled */
    if ((con->flags & CTRL_CF_COALESCE) && ctrl_IsMotion(ev->type))
    {
        UWORD i, idx = con->head;

        for (i = 0; i < con->count; i++)
        {
            struct pHidd_Controller_Event *q = &con->ring[idx];

            if (q->type == ev->type && q->device_id == ev->device_id && q->code == ev->code)
            {
                q->value = ev->value;
                q->sequence = ev->sequence;
                q->timestamp = ev->timestamp;
                return;
            }
            idx = (idx + 1) % con->depth;
        }
    }

    if (con->count >= con->depth)
    {
        con->flags |= CTRL_CF_OVERFLOW;
        con->dropped++;
        return;
    }
    con->ring[con->tail] = *ev;
    con->tail = (con->tail + 1) % con->depth;
    con->count++;
}

static struct Hidd_Controller_EventMsg *ctrl_MsgAlloc(struct ControllerConsumer *con)
{
    /* called with interrupts disabled; messages replied with a NULL reply port become NT_FREEMSG */
    UWORD i;

    for (i = 0; i < con->depth; i++)
    {
        struct Hidd_Controller_EventMsg *m = &con->msgs[i];

        if (m->msg.mn_Node.ln_Type == NT_FREEMSG)
        {
            m->msg.mn_Node.ln_Type = NT_MESSAGE;
            return m;
        }
    }
    return NULL;
}

/* Deliver one event to one consumer (filters already applied) */
static void ctrl_DeliverTo(OOP_Class *cl, struct ControllerConsumer *con, const struct pHidd_Controller_Event *ev)
{
    switch (con->mode)
    {
    case CTRL_MODE_CALLBACK:
        if (con->cb)
            con->cb(con->cbdata, (InputIrqData_t)ev);
        break;

    case CTRL_MODE_SIGNAL:
        Disable();
        ctrl_RingPush(con, ev);
        Enable();
        if (con->task)
            Signal(con->task, con->sigmask);
        break;

    case CTRL_MODE_PORT:
    {
        struct Hidd_Controller_EventMsg *m;

        Disable();
        if (con->flags & CTRL_CF_OVERFLOW)
        {
            m = ctrl_MsgAlloc(con);
            if (m)
            {
                m->ev = *ev;
                m->ev.type = vHidd_Controller_Overflow;
                m->ev.code = 0;
                m->ev.std = vHidd_Controller_Std_None;
                m->ev.flags = 0;
                m->ev.value = con->dropped;
                con->flags &= ~CTRL_CF_OVERFLOW;
                con->dropped = 0;
                Enable();
                PutMsg(con->port, &m->msg);
                Disable();
            }
        }
        m = ctrl_MsgAlloc(con);
        if (!m)
        {
            con->flags |= CTRL_CF_OVERFLOW;
            con->dropped++;
            Enable();
            break;
        }
        m->ev = *ev;
        Enable();
        PutMsg(con->port, &m->msg);
        break;
    }
    }
}

static inline BOOL ctrl_ConsumerWants(const struct ControllerConsumer *con, const struct pHidd_Controller_Event *ev)
{
    if (!(con->eventmask & vHidd_Controller_EventMask(ev->type)))
        return FALSE;
    if (con->device_id && con->device_id != ev->device_id &&
        ev->type != vHidd_Controller_DeviceAdded && ev->type != vHidd_Controller_DeviceRemoved)
        return FALSE;
    return TRUE;
}

/*
 * Fan an event out to the device bound consumers and the subsystem wide
 * consumers. Consumer pointers are collected under Disable() so that the
 * callbacks themselves run with interrupts in whatever state the producer
 * left them.
 */
void ctrl_DeliverEvent(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerDevice *dev,
                       const struct pHidd_Controller_Event *ev)
{
    struct ControllerConsumer *list[CTRL_FANOUT_MAX];
    struct ControllerConsumer *con;
    ULONG n = 0, i;

    Disable();
    if (dev)
    {
        ForeachNode(&dev->consumers, con)
        {
            if (n < CTRL_FANOUT_MAX && ctrl_ConsumerWants(con, ev))
                list[n++] = con;
        }
    }
    if (hw)
    {
        ForeachNode(&hw->consumers, con)
        {
            if (n < CTRL_FANOUT_MAX && ctrl_ConsumerWants(con, ev))
                list[n++] = con;
        }
    }
    Enable();

    for (i = 0; i < n; i++)
        ctrl_DeliverTo(cl, list[i], ev);
}

void ctrl_SendDeviceEvent(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerDevice *dev, UWORD type, LONG value)
{
    struct pHidd_Controller_Event ev;

    ev.type = type;
    ev.device_id = dev->id;
    ev.code = 0;
    ev.std = vHidd_Controller_Std_None;
    ev.flags = 0;
    ev.reserved = 0;
    ev.value = value;
    ev.sequence = dev->rd.sequence;
    ev.timestamp = ctrl_Now(cl);
    ctrl_DeliverEvent(cl, hw, dev, &ev);
}

void ctrl_ConsumerAttach(OOP_Class *cl, struct ControllerHWData *hw, struct ControllerConsumer *con)
{
    struct MinList *list = con->dev ? &con->dev->consumers : &hw->consumers;

    Disable();
    ADDTAIL(list, &con->node);
    con->flags |= CTRL_CF_ATTACHED;
    Enable();

    /* Initial salvo: one DeviceAdded per connected device for subsystem wide consumers */
    if (!con->dev && (con->eventmask & vHidd_Controller_EventMask(vHidd_Controller_DeviceAdded)))
    {
        UWORD ids[64];
        ULONG n = 0, i;
        struct ControllerDevice *dev;

        Disable();
        ForeachNode(&hw->devices, dev)
        {
            if (n < 64 && (dev->flags & CTRL_DF_REGISTERED))
                ids[n++] = dev->id;
        }
        Enable();

        for (i = 0; i < n; i++)
        {
            struct pHidd_Controller_Event ev;

            ev.type = vHidd_Controller_DeviceAdded;
            ev.device_id = ids[i];
            ev.code = 0;
            ev.std = vHidd_Controller_Std_None;
            ev.flags = 0;
            ev.reserved = 0;
            ev.value = 0;
            ev.sequence = 0;
            ev.timestamp = ctrl_Now(cl);
            ctrl_DeliverTo(cl, con, &ev);
        }
    }
}

void ctrl_ConsumerDetach(OOP_Class *cl, struct ControllerConsumer *con)
{
    Disable();
    if (con->flags & CTRL_CF_ATTACHED)
    {
        REMOVE(&con->node);
        con->flags &= ~CTRL_CF_ATTACHED;
    }
    con->dev = NULL;
    Enable();
}

/* Device going away: tell its bound consumers and unlink them */
void ctrl_DetachDeviceConsumers(OOP_Class *cl, struct ControllerDevice *dev)
{
    struct ControllerConsumer *con, *tmp;
    struct pHidd_Controller_Event ev;

    ev.type = vHidd_Controller_DeviceRemoved;
    ev.device_id = dev->id;
    ev.code = 0;
    ev.std = vHidd_Controller_Std_None;
    ev.flags = 0;
    ev.reserved = 0;
    ev.value = 0;
    ev.sequence = dev->rd.sequence;
    ev.timestamp = ctrl_Now(cl);

    for (;;)
    {
        Disable();
        con = (struct ControllerConsumer *)dev->consumers.mlh_Head;
        if (!con->node.mln_Succ)
        {
            Enable();
            break;
        }
        REMOVE(&con->node);
        con->flags &= ~CTRL_CF_ATTACHED;
        con->dev = NULL;
        Enable();

        if (con->eventmask & vHidd_Controller_EventMask(vHidd_Controller_DeviceRemoved))
            ctrl_DeliverTo(cl, con, &ev);
    }
    (void)tmp;
}

BOOL ctrl_ConsumerGetEvent(OOP_Class *cl, struct ControllerConsumer *con, struct pHidd_Controller_Event *dst)
{
    BOOL got = FALSE;

    if (con->mode != CTRL_MODE_SIGNAL || !con->ring)
        return FALSE;

    Disable();
    if (con->count)
    {
        *dst = con->ring[con->head];
        con->head = (con->head + 1) % con->depth;
        con->count--;
        got = TRUE;
    }
    else if (con->flags & CTRL_CF_OVERFLOW)
    {
        dst->type = vHidd_Controller_Overflow;
        dst->device_id = con->device_id;
        dst->code = 0;
        dst->std = vHidd_Controller_Std_None;
        dst->flags = 0;
        dst->reserved = 0;
        dst->value = con->dropped;
        dst->sequence = 0;
        dst->timestamp = 0;
        con->flags &= ~CTRL_CF_OVERFLOW;
        con->dropped = 0;
        got = TRUE;
    }
    Enable();

    return got;
}

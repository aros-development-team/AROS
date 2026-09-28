/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: reading cache (seqlock), raw report ingestion and event generation
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <hidd/controller.h>

#include "controller_intern.h"

#define SysBase   ((struct ExecBase *)(CSD(cl)->cs_SysBase))
#define TimerBase (CSD(cl)->cs_TimerBase)

/* Compiler/CPU barriers for the seqlock */
#define ctrl_barrier() __sync_synchronize()

UQUAD ctrl_Now(OOP_Class *cl)
{
    struct EClockVal ev;

    if (!TimerBase)
        return 0;
    ReadEClock(&ev);
    return ((UQUAD)ev.ev_hi << 32) | ev.ev_lo;
}

void ctrl_ReadingInit(struct ControllerDevice *dev)
{
    UWORD i;

    dev->seq = 0;
    dev->rd.size = sizeof(struct pHidd_Controller_Reading);
    dev->rd.device_id = dev->id;
    dev->rd.flags = vHidd_Controller_RF_Connected;
    if (dev->flags & CTRL_DF_TRUNCATED)
        dev->rd.flags |= vHidd_Controller_RF_Truncated;
    dev->rd.sequence = 0;
    dev->rd.timestamp = 0;
    dev->rd.button_count = dev->nbuttons;
    dev->rd.axis_count = dev->naxes;
    dev->rd.hat_count = dev->nhats;
    dev->rd.buttons[0] = dev->rd.buttons[1] = 0;
    for (i = 0; i < HIDD_CONTROLLER_MAX_AXES; i++)
        dev->rd.axes[i] = 0;
    for (i = 0; i < HIDD_CONTROLLER_MAX_HATS; i++)
        dev->rd.hats[i] = 0;
    dev->rd.gp_buttons = 0;
    for (i = 0; i < vHidd_Controller_GPA_AxisCount; i++)
        dev->rd.gp_axes[i] = 0;
    for (i = 0; i < vHidd_Controller_ARCH_AxisCount; i++)
        dev->rd.arch_axes[i] = 0;
}

/*
 * Seqlock read side. Safe from any context on any CPU; never blocks the
 * writer. Writers run with interrupts disabled, so on a single CPU a reader
 * can never observe an odd sequence; on SMP it simply retries.
 */
ULONG ctrl_CopyReading(OOP_Class *cl, const struct ControllerDevice *dev, struct pHidd_Controller_Reading *dst)
{
    ULONG s1, s2;
    ULONG want = dst->size;

    if (want == 0 || want > sizeof(struct pHidd_Controller_Reading))
        want = sizeof(struct pHidd_Controller_Reading);

    do
    {
        s1 = dev->seq;
        ctrl_barrier();
        if (s1 & 1)
            continue;
        CopyMem((APTR)&dev->rd, dst, want);
        ctrl_barrier();
        s2 = dev->seq;
    } while (s1 != s2);

    dst->size = want;
    return dst->sequence;
}

static inline void ctrl_WriteBegin(OOP_Class *cl, struct ControllerDevice *dev)
{
    Disable();
    dev->seq++;
    ctrl_barrier();
}

static inline void ctrl_WriteEnd(OOP_Class *cl, struct ControllerDevice *dev)
{
    ctrl_barrier();
    dev->seq++;
    Enable();
}

/* Event record collected during a write, flushed after the seqlock is released */
struct ctrl_pending
{
    UWORD type;
    UWORD code;
    UWORD std;
    UWORD flags;
    LONG  value;
};
#define CTRL_PENDING_MAX (HIDD_CONTROLLER_MAX_BUTTONS + HIDD_CONTROLLER_MAX_AXES + HIDD_CONTROLLER_MAX_HATS)

static void ctrl_Flush(OOP_Class *cl, struct ControllerDevice *dev, struct ctrl_pending *pend, ULONG n,
                       ULONG sequence, UQUAD ts)
{
    struct pHidd_Controller_Event ev;
    ULONG i;

    ev.device_id = dev->id;
    ev.reserved = 0;
    ev.sequence = sequence;
    ev.timestamp = ts;

    for (i = 0; i < n; i++)
    {
        ev.type  = pend[i].type;
        ev.code  = pend[i].code;
        ev.std   = pend[i].std;
        ev.flags = pend[i].flags;
        ev.value = pend[i].value;
        ctrl_DeliverEvent(cl, dev->hw, dev, &ev);
    }

    ev.type = vHidd_Controller_Frame;
    ev.code = 0;
    ev.std = vHidd_Controller_Std_None;
    ev.flags = 0;
    ev.value = 0;
    ctrl_DeliverEvent(cl, dev->hw, dev, &ev);
}

static inline void ctrl_Queue(struct ctrl_pending *pend, ULONG *n, UWORD type, UWORD code, UWORD std,
                              UWORD flags, LONG value)
{
    if (*n >= CTRL_PENDING_MAX)
        return;
    pend[*n].type = type;
    pend[*n].code = code;
    pend[*n].std = std;
    pend[*n].flags = flags | ((std != vHidd_Controller_Std_None) ? vHidd_Controller_EF_Standard : 0);
    pend[*n].value = value;
    (*n)++;
}

/*
 * Ingest a raw report. Runs in the producer's context (interrupt or task).
 */
void ctrl_PushReport(OOP_Class *cl, struct ControllerDevice *dev, const struct pHidd_Controller_RawReport *r)
{
    struct ctrl_pending pend[CTRL_PENDING_MAX];
    ULONG n = 0;
    ULONG sequence;
    UQUAD ts = r->timestamp ? r->timestamp : ctrl_Now(cl);
    UWORD i;

    ctrl_WriteBegin(cl, dev);

    if (r->valid & vHidd_Controller_RR_Buttons)
    {
        for (i = 0; i < dev->nbuttons; i++)
        {
            ULONG word = i >> 5, bit = 1UL << (i & 31);
            BOOL now = (r->buttons[word] & bit) != 0;
            BOOL was = (dev->rd.buttons[word] & bit) != 0;

            if (now != was)
            {
                if (now) dev->rd.buttons[word] |= bit;
                else     dev->rd.buttons[word] &= ~bit;
                ctrl_Queue(pend, &n, now ? vHidd_Controller_Press : vHidd_Controller_Release,
                           i, dev->raw_btn_std[i], 0, now);
            }
        }
    }

    if (r->valid & vHidd_Controller_RR_Axes)
    {
        for (i = 0; i < dev->naxes; i++)
        {
            if (ctrl_AxisChanged(dev, i, r->axes[i]))
            {
                WORD v = ctrl_NormaliseAxis(dev, i, r->axes[i]);

                if (v != dev->rd.axes[i] || (dev->axes[i].flags & vHidd_Controller_CF_Relative))
                {
                    dev->rd.axes[i] = v;
                    ctrl_Queue(pend, &n, vHidd_Controller_AxisMotion, i, dev->raw_axis_std[i],
                               (dev->axes[i].flags & vHidd_Controller_CF_Unipolar) ? vHidd_Controller_EF_Unipolar : 0, v);
                }
            }
        }
    }

    if (r->valid & vHidd_Controller_RR_Hats)
    {
        for (i = 0; i < dev->nhats; i++)
        {
            UBYTE v = (r->valid & vHidd_Controller_RR_HatsMask) ? (UBYTE)(r->hats[i] & 0x0F)
                                                                 : ctrl_DecodeHat(&dev->hats[i], r->hats[i]);

            if (v != dev->rd.hats[i])
            {
                dev->rd.hats[i] = v;
                ctrl_Queue(pend, &n, vHidd_Controller_HatMotion, i, vHidd_Controller_Std_None, 0, v);
            }
        }
    }

    if (n)
    {
        ctrl_ComputeStandardView(dev);
        dev->rd.sequence++;
        dev->rd.timestamp = ts;
    }
    sequence = dev->rd.sequence;

    ctrl_WriteEnd(cl, dev);

    if (n)
        ctrl_Flush(cl, dev, pend, n, sequence, ts);
}

/* Single control update; builds a raw report around the cached state */
void ctrl_PushValue(OOP_Class *cl, struct ControllerDevice *dev, UBYTE kind, UBYTE index, LONG value, UQUAD ts)
{
    struct pHidd_Controller_RawReport r;
    UWORD i;

    r.timestamp = ts;
    r.valid = 0;

    switch (kind)
    {
    case vHidd_Controller_Ctl_Button:
        if (index >= dev->nbuttons) return;
        r.buttons[0] = dev->rd.buttons[0];
        r.buttons[1] = dev->rd.buttons[1];
        if (value) r.buttons[index >> 5] |=  (1UL << (index & 31));
        else       r.buttons[index >> 5] &= ~(1UL << (index & 31));
        r.valid = vHidd_Controller_RR_Buttons;
        break;

    case vHidd_Controller_Ctl_Axis:
        if (index >= dev->naxes) return;
        for (i = 0; i < HIDD_CONTROLLER_MAX_AXES; i++)
            r.axes[i] = dev->axis_raw[i];
        r.axes[index] = value;
        r.valid = vHidd_Controller_RR_Axes;
        break;

    case vHidd_Controller_Ctl_Hat:
        if (index >= dev->nhats) return;
        for (i = 0; i < HIDD_CONTROLLER_MAX_HATS; i++)
            r.hats[i] = dev->rd.hats[i];
        r.hats[index] = ctrl_DecodeHat(&dev->hats[index], value);
        r.valid = vHidd_Controller_RR_Hats | vHidd_Controller_RR_HatsMask;
        break;

    default:
        return;
    }

    ctrl_PushReport(cl, dev, &r);
}

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: control table import, axis normalisation, hat decoding
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <hidd/controller.h>

#include "controller_intern.h"

#define SysBase ((struct ExecBase *)(CSD(cl)->cs_SysBase))

static void ctrl_AxisDefaults(struct ControllerDevice *dev, UWORD idx)
{
    struct Hidd_Controller_ControlDesc *d = &dev->axes[idx];
    LONG range;

    if (d->max <= d->min)
    {
        /* degenerate range: assume signed 16 bit */
        d->min = -32768;
        d->max = 32767;
    }
    range = d->max - d->min;

    /* Linux hid-input defaults for game collections: flat = range>>4, fuzz = range>>8 */
    dev->axis_flat[idx] = (d->flags & vHidd_Controller_CF_NoDeadzone) ? 0 :
                          (d->flat ? (LONG)d->flat : (range >> 4));
    dev->axis_fuzz[idx] = d->fuzz ? (LONG)d->fuzz : (range >> 8);
    if (d->flags & vHidd_Controller_CF_Relative)
    {
        dev->axis_flat[idx] = 0;
        dev->axis_fuzz[idx] = 0;
    }
    /* rest position: centre for bipolar, min for unipolar */
    dev->axis_raw[idx] = (d->flags & vHidd_Controller_CF_Unipolar) ? d->min : d->min + range / 2;
}

/*
 * Copy the driver supplied tables into the device, validating and truncating.
 * Descriptors with indices out of order are accepted; gaps are filled with
 * empty descriptors of the same kind.
 */
BOOL ctrl_ImportControls(OOP_Class *cl, struct ControllerDevice *dev,
                         const struct Hidd_Controller_ControlDesc *table,
                         const struct Hidd_Controller_OutputDesc *outputs)
{
    UWORD i;

    dev->nbuttons = dev->naxes = dev->nhats = dev->ntouchpads = dev->nsensors = 0;
    dev->noutputs = 0;
    dev->caps = 0;

    for (i = 0; i < HIDD_CONTROLLER_MAX_BUTTONS; i++)
    {
        dev->buttons[i].kind = vHidd_Controller_Ctl_Button;
        dev->buttons[i].index = i;
        dev->buttons[i].label = vHidd_Controller_Label_Numbered + i + 1;
    }
    for (i = 0; i < HIDD_CONTROLLER_MAX_AXES; i++)
    {
        dev->axes[i].kind = vHidd_Controller_Ctl_Axis;
        dev->axes[i].index = i;
        dev->axes[i].min = -32768;
        dev->axes[i].max = 32767;
    }
    for (i = 0; i < HIDD_CONTROLLER_MAX_HATS; i++)
    {
        dev->hats[i].kind = vHidd_Controller_Ctl_Hat;
        dev->hats[i].index = i;
        dev->hats[i].min = 0;
        dev->hats[i].max = 7;
    }

    if (table)
    {
        for (; table->kind != vHidd_Controller_Ctl_End; table++)
        {
            struct Hidd_Controller_ControlDesc *dst = NULL;
            UWORD *count = NULL, max = 0;

            switch (table->kind)
            {
            case vHidd_Controller_Ctl_Button:
                dst = dev->buttons; count = &dev->nbuttons; max = HIDD_CONTROLLER_MAX_BUTTONS; break;
            case vHidd_Controller_Ctl_Axis:
                dst = dev->axes; count = &dev->naxes; max = HIDD_CONTROLLER_MAX_AXES; break;
            case vHidd_Controller_Ctl_Hat:
                dst = dev->hats; count = &dev->nhats; max = HIDD_CONTROLLER_MAX_HATS; break;
            case vHidd_Controller_Ctl_Touchpad:
                dst = dev->touchpads; count = &dev->ntouchpads; max = HIDD_CONTROLLER_MAX_TOUCHPADS; break;
            case vHidd_Controller_Ctl_Sensor:
                dst = dev->sensors; count = &dev->nsensors; max = HIDD_CONTROLLER_MAX_SENSORS; break;
            default:
                D(bug("[Controller] %s: unknown control kind %u\n", __func__, table->kind));
                continue;
            }

            if (table->index >= max)
            {
                dev->flags |= CTRL_DF_TRUNCATED;
                continue;
            }
            dst[table->index] = *table;
            if (table->index + 1 > *count)
                *count = table->index + 1;
        }
    }

    for (i = 0; i < dev->naxes; i++)
    {
        ctrl_AxisDefaults(dev, i);
        if (dev->axes[i].flags & vHidd_Controller_CF_Unipolar)
            dev->caps |= vHidd_Controller_Cap_Triggers;
    }
    for (i = 0; i < dev->nhats; i++)
    {
        if (!(dev->hats[i].flags & vHidd_Controller_CF_HatMask) && dev->hats[i].max <= dev->hats[i].min)
        {
            dev->hats[i].min = 0;
            dev->hats[i].max = 7;
        }
    }

    if (outputs)
    {
        for (; outputs->kind != vHidd_Controller_Out_End; outputs++)
        {
            if (dev->noutputs >= HIDD_CONTROLLER_MAX_OUTPUTS)
            {
                dev->flags |= CTRL_DF_TRUNCATED;
                break;
            }
            dev->outputs[dev->noutputs++] = *outputs;
            switch (outputs->kind)
            {
            case vHidd_Controller_Out_RumbleLow:
            case vHidd_Controller_Out_RumbleHigh:
                dev->caps |= vHidd_Controller_Cap_Rumble; break;
            case vHidd_Controller_Out_RumbleLeftTrigger:
            case vHidd_Controller_Out_RumbleRightTrigger:
                dev->caps |= vHidd_Controller_Cap_TriggerRumble; break;
            case vHidd_Controller_Out_LEDRGB:
                dev->caps |= vHidd_Controller_Cap_LEDRGB; break;
            case vHidd_Controller_Out_LEDPlayer:
                dev->caps |= vHidd_Controller_Cap_LEDPlayer; break;
            case vHidd_Controller_Out_LEDMono:
                dev->caps |= vHidd_Controller_Cap_LEDMono; break;
            case vHidd_Controller_Out_FFMotor:
                dev->caps |= vHidd_Controller_Cap_Effects; break;
            case vHidd_Controller_Out_RawEffect:
            case vHidd_Controller_Out_TriggerEffect:
                dev->caps |= vHidd_Controller_Cap_RawEffect; break;
            }
        }
    }

    if (dev->nbuttons)   dev->caps |= vHidd_Controller_Cap_Buttons;
    if (dev->naxes)      dev->caps |= vHidd_Controller_Cap_Axes;
    if (dev->nhats)      dev->caps |= vHidd_Controller_Cap_Hats;
    if (dev->ntouchpads) dev->caps |= vHidd_Controller_Cap_Touchpad;
    if (dev->nsensors)   dev->caps |= vHidd_Controller_Cap_Sensors;
    if (dev->flags & CTRL_DF_TRUNCATED) dev->caps |= vHidd_Controller_Cap_Truncated;

    D(bug("[Controller] %s: %u buttons, %u axes, %u hats, %u touchpads, %u sensors, %u outputs, caps 0x%08x\n",
          __func__, dev->nbuttons, dev->naxes, dev->nhats, dev->ntouchpads, dev->nsensors, dev->noutputs, dev->caps));

    return TRUE;
}

/*
 * Apply fuzz to an incoming raw axis value; returns TRUE when the value is
 * accepted as a change (dev->axis_raw updated).
 */
BOOL ctrl_AxisChanged(struct ControllerDevice *dev, UWORD idx, LONG raw)
{
    LONG old = dev->axis_raw[idx];
    LONG diff = raw - old;
    LONG fuzz = dev->axis_fuzz[idx];

    if (dev->axes[idx].flags & vHidd_Controller_CF_Relative)
    {
        dev->axis_raw[idx] = raw;
        return raw != 0;
    }
    if (diff < 0)
        diff = -diff;
    if (diff == 0)
        return FALSE;
    if (fuzz > 0 && diff < (fuzz >> 1))
        return FALSE;
    dev->axis_raw[idx] = raw;
    return TRUE;
}

/* Normalise a raw axis value into the public WORD range */
WORD ctrl_NormaliseAxis(const struct ControllerDevice *dev, UWORD idx, LONG raw)
{
    const struct Hidd_Controller_ControlDesc *d = &dev->axes[idx];
    LONG range = d->max - d->min;
    LONG flat = dev->axis_flat[idx];
    LONG v;

    if (raw < d->min) raw = d->min;
    if (raw > d->max) raw = d->max;

    if (d->flags & vHidd_Controller_CF_Relative)
    {
        v = raw;
        if (v < HIDD_CONTROLLER_AXIS_MIN) v = HIDD_CONTROLLER_AXIS_MIN;
        if (v > HIDD_CONTROLLER_AXIS_MAX) v = HIDD_CONTROLLER_AXIS_MAX;
        return (WORD)((d->flags & vHidd_Controller_CF_Inverted) ? -v : v);
    }

    if (d->flags & vHidd_Controller_CF_Unipolar)
    {
        if (d->flags & vHidd_Controller_CF_Inverted)
            raw = d->max - (raw - d->min);
        if (flat && (raw - d->min) < flat)
            return 0;
        v = ((raw - d->min) * (LONG)HIDD_CONTROLLER_TRIGGER_MAX) / range;
        if (v > HIDD_CONTROLLER_TRIGGER_MAX) v = HIDD_CONTROLLER_TRIGGER_MAX;
        return (WORD)v;
    }
    else
    {
        LONG centre_raw = d->min + range / 2;
        LONG off = raw - centre_raw;

        if (flat && off > -flat && off < flat)
            return 0;
        /* 32-bit safe scaling: range is at most 65535 for sane devices but may be 24 bit */
        if (range <= 65535)
            v = ((raw - d->min) * 65535L) / range - 32768;
        else
            v = (LONG)(((QUAD)(raw - d->min) * 65535LL) / range) - 32768;
        if (v < HIDD_CONTROLLER_AXIS_MIN) v = HIDD_CONTROLLER_AXIS_MIN;
        if (v > HIDD_CONTROLLER_AXIS_MAX) v = HIDD_CONTROLLER_AXIS_MAX;
        if (d->flags & vHidd_Controller_CF_Inverted)
            v = (v == HIDD_CONTROLLER_AXIS_MIN) ? HIDD_CONTROLLER_AXIS_MAX : -v;
        return (WORD)v;
    }
}

/* Decode a raw hat value into a direction bitmask */
UBYTE ctrl_DecodeHat(const struct Hidd_Controller_ControlDesc *desc, LONG raw)
{
    static const UBYTE clockwise[8] =
    {
        vHidd_Controller_Hat_Up,
        vHidd_Controller_Hat_Up | vHidd_Controller_Hat_Right,
        vHidd_Controller_Hat_Right,
        vHidd_Controller_Hat_Right | vHidd_Controller_Hat_Down,
        vHidd_Controller_Hat_Down,
        vHidd_Controller_Hat_Down | vHidd_Controller_Hat_Left,
        vHidd_Controller_Hat_Left,
        vHidd_Controller_Hat_Left | vHidd_Controller_Hat_Up
    };

    if (desc->flags & vHidd_Controller_CF_HatMask)
        return (UBYTE)(raw & 0x0F);

    raw -= desc->min;
    if (raw < 0 || raw > 7)
        return vHidd_Controller_Hat_Center;
    return clockwise[raw];
}

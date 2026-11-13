/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CLID_Hidd_VirtualPad - a simulated controller device
*/

#define DEBUG 0
#include <aros/debug.h>

#include <hidd/hidd.h>
#include <hidd/controller.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "virtualpad_intern.h"

#define SysBase     ((struct ExecBase *)(VSD(cl)->cs_SysBase))
#define UtilityBase (VSD(cl)->cs_UtilityBase)
#define OOPBase     (VSD(cl)->cs_OOPBase)

static void vpad_Log(struct VirtualPadData *data, UWORD type, LONG a, LONG b, LONG c, LONG d)
{
    struct VirtualPad_Output *e = &data->log[data->log_head];

    e->type = type;
    e->reserved = 0;
    e->a = a; e->b = b; e->c = c; e->d = d;
    data->log_head = (data->log_head + 1) % VPAD_LOG_SIZE;
    if (data->log_count < VPAD_LOG_SIZE)
        data->log_count++;
}

OOP_Object *VirtualPad__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    CONST_STRPTR name = (CONST_STRPTR)GetTagData(aHidd_VirtualPad_Profile, 0, msg->attrList);
    const struct VirtualPad_Profile *p = vpad_FindProfile(name);

    if (!p)
    {
        D(bug("[VirtualPad] %s: unknown profile '%s'\n", __func__, name ? name : (CONST_STRPTR)"(null)"));
        return NULL;
    }

    {
        struct TagItem tags[] =
        {
            { aHidd_Name,                    (IPTR)p->name          },
            { aHidd_HardwareName,            (IPTR)p->hwname        },
            { aHidd_Controller_Manufacturer, (IPTR)p->manufacturer  },
            { aHidd_Controller_Serial,       (IPTR)p->serial        },
            { aHidd_Controller_VendorID,     p->vendor              },
            { aHidd_Controller_ProductID,    p->product             },
            { aHidd_Controller_Version,      p->version             },
            { aHidd_Controller_Bus,          p->bus                 },
            { aHidd_Controller_Family,       p->family              },
            { aHidd_Controller_Type,         p->type                },
            { aHidd_Controller_Connection,   p->connection          },
            { aHidd_Controller_Path,         (IPTR)"virtual"        },
            { aHidd_Controller_ControlTable, (IPTR)p->controls      },
            { aHidd_Controller_OutputTable,  (IPTR)p->outputs       },
            { p->bindings ? aHidd_Controller_BindingTable : TAG_IGNORE, (IPTR)p->bindings },
            { TAG_MORE,                      (IPTR)msg->attrList    }
        };
        struct pRoot_New newMsg = { .mID = msg->mID, .attrList = tags };

        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&newMsg);
    }

    if (o)
    {
        struct VirtualPadData *data = OOP_INST_DATA(cl, o);
        UWORD i;

        data->profile = p;
        data->log_head = data->log_count = 0;
        data->next_effect_id = 1;
        data->state.timestamp = 0;
        data->state.valid = 0;
        data->state.buttons[0] = data->state.buttons[1] = 0;
        /* rest positions: centre for bipolar axes, min for unipolar */
        for (i = 0; i < HIDD_CONTROLLER_MAX_AXES; i++)
        {
            const struct Hidd_Controller_ControlDesc *c;
            LONG rest = 0;
            for (c = p->controls; c->kind != vHidd_Controller_Ctl_End; c++)
            {
                if (c->kind == vHidd_Controller_Ctl_Axis && c->index == i)
                {
                    rest = (c->flags & vHidd_Controller_CF_Unipolar) ? c->min : c->min + (c->max - c->min) / 2;
                    break;
                }
            }
            data->state.axes[i] = rest;
        }
        for (i = 0; i < HIDD_CONTROLLER_MAX_HATS; i++)
            data->state.hats[i] = 8;    /* centred in 0..7 encoding */
    }
    return o;
}

VOID VirtualPad__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, msg);
}

VOID VirtualPad__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_IF_ATTR(msg->attrID, idx, HiddVirtualPadAB, num_Hidd_VirtualPad_Attrs))
    {
        switch (idx)
        {
        case aoHidd_VirtualPad_Profile:     *msg->storage = (IPTR)data->profile->name; return;
        case aoHidd_VirtualPad_Description: *msg->storage = (IPTR)data->profile->description; return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* Output methods: log, then let the base class record the request */

BOOL VirtualPad__Hidd_Controller__SetRumble(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetRumble *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    if (msg->params)
        vpad_Log(data, vVirtualPad_Out_Rumble, msg->params->low, msg->params->high,
                 msg->params->left_trigger, msg->params->right_trigger);
    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL VirtualPad__Hidd_Controller__SetLED(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetLED *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    vpad_Log(data, vVirtualPad_Out_LED, msg->red, msg->green, msg->blue, 0);
    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL VirtualPad__Hidd_Controller__SetPlayerIndex(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetPlayerIndex *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    vpad_Log(data, vVirtualPad_Out_PlayerIndex, msg->index, 0, 0, 0);
    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL VirtualPad__Hidd_Controller__SendEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SendEffect *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);
    IPTR caps = 0;

    OOP_GetAttr(o, aHidd_Controller_Capabilities, &caps);
    vpad_Log(data, vVirtualPad_Out_Effect, msg->size, (msg->data && msg->size) ? ((UBYTE *)msg->data)[0] : -1, 0, 0);
    return (caps & vHidd_Controller_Cap_RawEffect) ? TRUE : FALSE;
}

BOOL VirtualPad__Hidd_Controller__SetSensorsEnabled(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetSensorsEnabled *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    vpad_Log(data, vVirtualPad_Out_Sensors, msg->enable, 0, 0, 0);
    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

LONG VirtualPad__Hidd_Controller__UploadEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_UploadEffect *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);
    IPTR caps = 0;
    LONG id;

    OOP_GetAttr(o, aHidd_Controller_Capabilities, &caps);
    if (!(caps & vHidd_Controller_Cap_Effects) || !msg->effect)
        return -1;
    id = data->next_effect_id++;
    vpad_Log(data, vVirtualPad_Out_UploadEffect, msg->effect->type, msg->effect->motor, id, 0);
    return id;
}

BOOL VirtualPad__Hidd_Controller__PlayEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_PlayEffect *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    vpad_Log(data, vVirtualPad_Out_PlayEffect, msg->id, msg->loops, 0, 0);
    return msg->id > 0 && msg->id < data->next_effect_id;
}

BOOL VirtualPad__Hidd_Controller__StopEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_StopEffect *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    vpad_Log(data, vVirtualPad_Out_StopEffect, msg->id, 0, 0, 0);
    return msg->id > 0 && msg->id < data->next_effect_id;
}

BOOL VirtualPad__Hidd_Controller__RemoveEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_RemoveEffect *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    vpad_Log(data, vVirtualPad_Out_RemoveEffect, msg->id, 0, 0, 0);
    return msg->id > 0 && msg->id < data->next_effect_id;
}

/* Simulation input */

void VirtualPad__Hidd_VirtualPad__Feed(OOP_Class *cl, OOP_Object *o, struct pHidd_VirtualPad_Feed *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    switch (msg->kind)
    {
    case vHidd_Controller_Ctl_Button:
        if (msg->index < HIDD_CONTROLLER_MAX_BUTTONS)
        {
            if (msg->value) data->state.buttons[msg->index >> 5] |=  (1UL << (msg->index & 31));
            else            data->state.buttons[msg->index >> 5] &= ~(1UL << (msg->index & 31));
        }
        break;
    case vHidd_Controller_Ctl_Axis:
        if (msg->index < HIDD_CONTROLLER_MAX_AXES)
            data->state.axes[msg->index] = msg->value;
        break;
    case vHidd_Controller_Ctl_Hat:
        if (msg->index < HIDD_CONTROLLER_MAX_HATS)
            data->state.hats[msg->index] = (UBYTE)msg->value;
        break;
    }
    HIDD_Controller_PushValue(o, msg->kind, msg->index, msg->value, 0);
}

void VirtualPad__Hidd_VirtualPad__FeedReport(OOP_Class *cl, OOP_Object *o, struct pHidd_VirtualPad_FeedReport *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);

    if (!msg->report)
        return;
    data->state = *msg->report;
    HIDD_Controller_PushReport(o, msg->report);
}

void VirtualPad__Hidd_VirtualPad__FeedTouch(OOP_Class *cl, OOP_Object *o, struct pHidd_VirtualPad_FeedTouch *msg)
{
    HIDD_Controller_PushTouch(o, msg->pad, msg->finger, msg->state);
}

void VirtualPad__Hidd_VirtualPad__FeedSensor(OOP_Class *cl, OOP_Object *o, struct pHidd_VirtualPad_FeedSensor *msg)
{
    struct pHidd_Controller_SensorSample s;

    s.device_timestamp = 0;
    s.timestamp = 0;
    s.v[0] = msg->x;
    s.v[1] = msg->y;
    s.v[2] = msg->z;
    HIDD_Controller_PushSensor(o, msg->sensor, &s, 1);
}

void VirtualPad__Hidd_VirtualPad__FeedPower(OOP_Class *cl, OOP_Object *o, struct pHidd_VirtualPad_FeedPower *msg)
{
    struct pHidd_Controller_PowerInfo p;

    p.state = msg->state;
    p.percent = msg->percent;
    p.level = (msg->percent < 0) ? vHidd_Controller_Level_Unknown :
              (msg->percent < 5) ? vHidd_Controller_Level_Empty :
              (msg->percent < 30) ? vHidd_Controller_Level_Low :
              (msg->percent < 80) ? vHidd_Controller_Level_Medium : vHidd_Controller_Level_Full;
    p.reserved = 0;
    HIDD_Controller_PushPower(o, &p);
}

void VirtualPad__Hidd_VirtualPad__FeedConnected(OOP_Class *cl, OOP_Object *o, struct pHidd_VirtualPad_FeedConnected *msg)
{
    HIDD_Controller_SetConnected(o, msg->connected);
}

ULONG VirtualPad__Hidd_VirtualPad__GetOutputLog(OOP_Class *cl, OOP_Object *o, struct pHidd_VirtualPad_GetOutputLog *msg)
{
    struct VirtualPadData *data = OOP_INST_DATA(cl, o);
    ULONG n = 0, i;

    for (i = 0; i < data->log_count && n < msg->max; i++)
    {
        ULONG idx = (data->log_head + VPAD_LOG_SIZE - data->log_count + i) % VPAD_LOG_SIZE;
        msg->buffer[n++] = data->log[idx];
    }
    data->log_count = 0;
    return n;
}

/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: CLID_Hidd_Controller - device base class and consumer class
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <exec/nodes.h>
#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/controller.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "controller_intern.h"

#define SysBase     ((struct ExecBase *)(CSD(cl)->cs_SysBase))
#define UtilityBase (CSD(cl)->cs_UtilityBase)
#define OOPBase     (CSD(cl)->cs_OOPBase)

/*****************************************************************************************

    NAME
        --background_controllerclass--

    LOCATION
        CLID_Hidd_Controller

    NOTES
        Objects of this class play one of two roles.

        Device role: a hardware driver class derives from CLID_Hidd_Controller and
        is instantiated through moHW_AddDriver on the CLID_HW_Controller subsystem
        object. One object represents one physical controller. The driver supplies
        its identity and its control table at creation time (aHidd_Controller_*
        [I..] attributes) and afterwards pushes raw input with the
        Push methods of IID_Hidd_Controller. The base class normalises values, keeps a
        cached reading, evaluates the standard gamepad layout and delivers events.
        Output requests (rumble, LEDs, effects) are IID_Hidd_Controller methods the
        driver overrides; the driver calls the superclass method after acting on
        the hardware so that the last requested values are recorded.

        Consumer role: a client creates a plain CLID_Hidd_Controller object to
        receive events, either from one device (aHidd_Controller_Device or
        aHidd_Controller_DeviceFilter) or from the whole subsystem, which also
        delivers DeviceAdded/DeviceRemoved events including an initial salvo of
        DeviceAdded for devices already present. Delivery is by callback
        (aHidd_Input_IrqHandler, producer context), by task signal
        (aHidd_Controller_NotifyTask/NotifySignal, drain with moHidd_Controller_GetEvent)
        or by message port (aHidd_Controller_NotifyPort, reply the messages).

        Current state of any device is read with moHidd_Controller_GetReading from
        any context.

*****************************************************************************************/

static void ctrl_ParseDeviceTags(OOP_Class *cl, struct ControllerDevice *dev, struct TagItem *tags)
{
    struct TagItem *tag, *tstate = tags;

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

        if (IS_HIDDCONTROLLER_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
            case aoHidd_Controller_VendorID:     dev->vendor = (UWORD)tag->ti_Data; break;
            case aoHidd_Controller_ProductID:    dev->product = (UWORD)tag->ti_Data; break;
            case aoHidd_Controller_Version:      dev->version = (UWORD)tag->ti_Data; break;
            case aoHidd_Controller_Manufacturer: dev->manufacturer = (CONST_STRPTR)tag->ti_Data; break;
            case aoHidd_Controller_Serial:       dev->serial = (CONST_STRPTR)tag->ti_Data; break;
            case aoHidd_Controller_Bus:          dev->bus = (UBYTE)tag->ti_Data; break;
            case aoHidd_Controller_Path:         dev->path = (CONST_STRPTR)tag->ti_Data; break;
            case aoHidd_Controller_GUID:
                if (tag->ti_Data)
                {
                    CopyMem((APTR)tag->ti_Data, dev->guid, 16);
                    dev->flags |= CTRL_DF_GUIDGIVEN;
                }
                break;
            case aoHidd_Controller_Type:         dev->type = (UBYTE)tag->ti_Data; break;
            case aoHidd_Controller_Style:        dev->style = (UBYTE)tag->ti_Data; break;
            case aoHidd_Controller_Family:       dev->family = (UBYTE)tag->ti_Data; break;
            case aoHidd_Controller_Connection:   dev->connection = (UBYTE)tag->ti_Data; break;
            case aoHidd_Controller_BindingTable: dev->drv_bindings = (const struct Hidd_Controller_Binding *)tag->ti_Data; break;
            }
        }
    }
}

static BOOL ctrl_ParseConsumerTags(OOP_Class *cl, struct ControllerConsumer *con, struct TagItem *tags)
{
    struct TagItem *tag, *tstate = tags;
    OOP_Object *devobj = NULL;

    con->eventmask = vHidd_Controller_EventMask_All;
    con->depth = CTRL_DEFAULT_QUEUE_DEPTH;
    con->mode = CTRL_MODE_NONE;

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

        if (IS_HIDDCONTROLLER_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
            case aoHidd_Controller_Device:       devobj = (OOP_Object *)tag->ti_Data; break;
            case aoHidd_Controller_DeviceFilter: con->device_id = (UWORD)tag->ti_Data; break;
            case aoHidd_Controller_EventMask:    con->eventmask = (ULONG)tag->ti_Data; break;
            case aoHidd_Controller_NotifyTask:   con->task = (struct Task *)tag->ti_Data; break;
            case aoHidd_Controller_NotifySignal: con->sigmask = 1UL << (tag->ti_Data & 31); break;
            case aoHidd_Controller_NotifyPort:   con->port = (struct MsgPort *)tag->ti_Data; break;
            case aoHidd_Controller_QueueDepth:
                con->depth = (UWORD)tag->ti_Data;
                if (con->depth == 0) con->depth = CTRL_DEFAULT_QUEUE_DEPTH;
                if (con->depth > CTRL_MAX_QUEUE_DEPTH) con->depth = CTRL_MAX_QUEUE_DEPTH;
                break;
            case aoHidd_Controller_Coalesce:
                if (tag->ti_Data) con->flags |= CTRL_CF_COALESCE;
                else con->flags &= ~CTRL_CF_COALESCE;
                break;
            }
        }
        else if (IS_HIDDINPUT_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
            case aoHidd_Input_IrqHandler:     con->cb = (InputIrqCallBack_t)tag->ti_Data; break;
            case aoHidd_Input_IrqHandlerData: con->cbdata = (APTR)tag->ti_Data; break;
            }
        }
    }

    if (devobj)
    {
        con->dev = ctrl_Device(cl, devobj);
        if (!con->dev)
            return FALSE;
        con->device_id = con->dev->id;
    }
    else if (con->device_id)
    {
        struct ControllerHWData *hw = OOP_INST_DATA(CSD(cl)->hwClass, CSD(cl)->hwObject);
        con->dev = ctrl_FindDevice(cl, hw, con->device_id);
        if (!con->dev)
            return FALSE;
    }

    if (con->port)
        con->mode = CTRL_MODE_PORT;
    else if (con->task)
        con->mode = CTRL_MODE_SIGNAL;
    else if (con->cb)
        con->mode = CTRL_MODE_CALLBACK;

    return TRUE;
}

OOP_Object *Controller__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem newTags[] =
    {
        { aHidd_Input_Subsystem, (IPTR)CSD(cl)->hwObject },
        { TAG_MORE,              (IPTR)msg->attrList     }
    };
    struct pRoot_New newMsg = { .mID = msg->mID, .attrList = newTags };
    struct TagItem *irqTag;
    BOOL isDevice = (GetTagData(aHW_Input_ConsumerList, 0, msg->attrList) != 0);
    Tag savedTag = 0;

    D(bug("[Controller] %s(%s)\n", __func__, isDevice ? "device" : "consumer"));

    /*
     * Consumers manage their own delivery; keep the callback away from the
     * generic input class so it is not registered on the raw input stream.
     */
    irqTag = FindTagItem(aHidd_Input_IrqHandler, msg->attrList);
    if (!isDevice && irqTag)
    {
        savedTag = irqTag->ti_Tag;
        irqTag->ti_Tag = TAG_IGNORE;
    }

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&newMsg);

    if (!isDevice && irqTag)
        irqTag->ti_Tag = savedTag;

    if (!o)
        return NULL;

    {
        struct ControllerInstData *data = OOP_INST_DATA(cl, o);
        struct ControllerHWData *hw = OOP_INST_DATA(CSD(cl)->hwClass, CSD(cl)->hwObject);

        if (isDevice)
        {
            struct ControllerDevice *dev = &data->u.dev;
            IPTR val = 0;
            const struct Hidd_Controller_ControlDesc *ctable;
            const struct Hidd_Controller_OutputDesc *otable;

            data->role = CTRL_ROLE_DEVICE;
            dev->obj = o;
            dev->hw = hw;
            dev->legacy_port = -1;
            dev->player_index = -1;
            dev->connected = TRUE;
            NEWLIST(&dev->consumers);

            OOP_GetAttr(o, aHidd_Name, &val);
            dev->name = (CONST_STRPTR)val;
            val = 0;
            OOP_GetAttr(o, aHidd_HardwareName, &val);
            dev->hwname = (CONST_STRPTR)val;
            if (!dev->hwname)
                dev->hwname = dev->name;

            ctrl_ParseDeviceTags(cl, dev, msg->attrList);
            ctable = (const struct Hidd_Controller_ControlDesc *)GetTagData(aHidd_Controller_ControlTable, 0, msg->attrList);
            otable = (const struct Hidd_Controller_OutputDesc *)GetTagData(aHidd_Controller_OutputTable, 0, msg->attrList);
            ctrl_ImportControls(cl, dev, ctable, otable);
            if (!(dev->flags & CTRL_DF_GUIDGIVEN))
                ctrl_ComputeGUID(dev);
            dev->power.state = vHidd_Controller_Power_Unknown;
            dev->power.percent = -1;
            ctrl_ReadingInit(dev);
        }
        else
        {
            struct ControllerConsumer *con = &data->u.con;

            data->role = CTRL_ROLE_CONSUMER;
            con->obj = o;
            if (!ctrl_ParseConsumerTags(cl, con, msg->attrList))
            {
                D(bug("[Controller] %s: consumer creation failed (device not found)\n", __func__));
                goto fail;
            }
            if (con->mode == CTRL_MODE_SIGNAL)
            {
                con->ring = AllocVec(con->depth * sizeof(struct pHidd_Controller_Event), MEMF_PUBLIC | MEMF_CLEAR);
                if (!con->ring)
                    goto fail;
            }
            else if (con->mode == CTRL_MODE_PORT)
            {
                UWORD i;
                con->msgs = AllocVec(con->depth * sizeof(struct Hidd_Controller_EventMsg), MEMF_PUBLIC | MEMF_CLEAR);
                if (!con->msgs)
                    goto fail;
                for (i = 0; i < con->depth; i++)
                {
                    con->msgs[i].msg.mn_Node.ln_Type = NT_FREEMSG;
                    con->msgs[i].msg.mn_ReplyPort = NULL;
                    con->msgs[i].msg.mn_Length = sizeof(struct Hidd_Controller_EventMsg);
                }
            }
            if (con->mode != CTRL_MODE_NONE)
                ctrl_ConsumerAttach(cl, hw, con);
        }
    }
    return o;

fail:
    {
        OOP_MethodID dispose_mid = msg->mID - moRoot_New + moRoot_Dispose;
        struct ControllerInstData *data = OOP_INST_DATA(cl, o);
        data->role = CTRL_ROLE_NONE;
        OOP_DoSuperMethod(cl, o, &dispose_mid);
    }
    return NULL;
}

VOID Controller__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ControllerInstData *data = OOP_INST_DATA(cl, o);

    D(bug("[Controller] %s(0x%p)\n", __func__, o));

    if (data->role == CTRL_ROLE_CONSUMER)
    {
        struct ControllerConsumer *con = &data->u.con;

        ctrl_ConsumerDetach(cl, con);
        if (con->ring) FreeVec(con->ring);
        if (con->msgs) FreeVec(con->msgs);
        con->ring = NULL;
        con->msgs = NULL;
    }
    else if (data->role == CTRL_ROLE_DEVICE)
    {
        ctrl_FreeMapping(cl, &data->u.dev);
    }
    data->role = CTRL_ROLE_NONE;

    OOP_DoSuperMethod(cl, o, msg);
}

VOID Controller__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ControllerInstData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_HIDDCONTROLLER_ATTR(msg->attrID, idx))
    {
        if (data->role == CTRL_ROLE_DEVICE)
        {
            struct ControllerDevice *dev = &data->u.dev;

            switch (idx)
            {
            case aoHidd_Controller_DeviceID:      *msg->storage = dev->id; return;
            case aoHidd_Controller_VendorID:      *msg->storage = dev->vendor; return;
            case aoHidd_Controller_ProductID:     *msg->storage = dev->product; return;
            case aoHidd_Controller_Version:       *msg->storage = dev->version; return;
            case aoHidd_Controller_Manufacturer:  *msg->storage = (IPTR)dev->manufacturer; return;
            case aoHidd_Controller_Serial:        *msg->storage = (IPTR)dev->serial; return;
            case aoHidd_Controller_Bus:           *msg->storage = dev->bus; return;
            case aoHidd_Controller_Path:          *msg->storage = (IPTR)dev->path; return;
            case aoHidd_Controller_GUID:          *msg->storage = (IPTR)dev->guid; return;
            case aoHidd_Controller_Type:          *msg->storage = dev->type; return;
            case aoHidd_Controller_Style:         *msg->storage = dev->style; return;
            case aoHidd_Controller_Family:        *msg->storage = dev->family; return;
            case aoHidd_Controller_Connection:    *msg->storage = dev->connection; return;
            case aoHidd_Controller_PlayerIndex:   *msg->storage = (IPTR)(LONG)dev->player_index; return;
            case aoHidd_Controller_LegacyPort:    *msg->storage = (IPTR)(LONG)dev->legacy_port; return;
            case aoHidd_Controller_Connected:     *msg->storage = dev->connected; return;
            case aoHidd_Controller_ButtonCount:   *msg->storage = dev->nbuttons; return;
            case aoHidd_Controller_AxisCount:     *msg->storage = dev->naxes; return;
            case aoHidd_Controller_HatCount:      *msg->storage = dev->nhats; return;
            case aoHidd_Controller_TouchpadCount: *msg->storage = dev->ntouchpads; return;
            case aoHidd_Controller_SensorCount:   *msg->storage = dev->nsensors; return;
            case aoHidd_Controller_OutputCount:   *msg->storage = dev->noutputs; return;
            case aoHidd_Controller_Capabilities:  *msg->storage = dev->caps; return;
            case aoHidd_Controller_Sequence:      *msg->storage = dev->rd.sequence; return;
            case aoHidd_Controller_SensorsEnabled:*msg->storage = dev->sensors_enabled; return;
            }
        }
        else if (data->role == CTRL_ROLE_CONSUMER)
        {
            struct ControllerConsumer *con = &data->u.con;

            switch (idx)
            {
            case aoHidd_Controller_DeviceFilter:  *msg->storage = con->device_id; return;
            case aoHidd_Controller_EventMask:     *msg->storage = con->eventmask; return;
            case aoHidd_Controller_QueueDepth:    *msg->storage = con->depth; return;
            case aoHidd_Controller_Coalesce:      *msg->storage = (con->flags & CTRL_CF_COALESCE) ? TRUE : FALSE; return;
            case aoHidd_Controller_Connected:     *msg->storage = (con->dev != NULL); return;
            }
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID Controller__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct ControllerInstData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->attrList;

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

        if (IS_HIDDCONTROLLER_ATTR(tag->ti_Tag, idx))
        {
            if (data->role == CTRL_ROLE_DEVICE)
            {
                switch (idx)
                {
                case aoHidd_Controller_PlayerIndex:
                    HIDD_Controller_SetPlayerIndex(o, (WORD)tag->ti_Data);
                    break;
                case aoHidd_Controller_SensorsEnabled:
                    HIDD_Controller_SetSensorsEnabled(o, tag->ti_Data ? TRUE : FALSE);
                    break;
                }
            }
            else if (data->role == CTRL_ROLE_CONSUMER)
            {
                switch (idx)
                {
                case aoHidd_Controller_EventMask:
                    data->u.con.eventmask = (ULONG)tag->ti_Data;
                    break;
                }
            }
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*****************************************************************************************
    IID_Hidd_Controller - input side
*****************************************************************************************/

ULONG Controller__Hidd_Controller__GetReading(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetReading *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev || !msg->dst)
        return 0;
    return ctrl_CopyReading(cl, dev, msg->dst);
}

BOOL Controller__Hidd_Controller__GetControlInfo(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetControlInfo *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);
    const struct Hidd_Controller_ControlDesc *d = NULL;

    if (!dev || !msg->dst)
        return FALSE;

    switch (msg->kind)
    {
    case vHidd_Controller_Ctl_Button:   if (msg->index < dev->nbuttons)   d = &dev->buttons[msg->index];   break;
    case vHidd_Controller_Ctl_Axis:     if (msg->index < dev->naxes)      d = &dev->axes[msg->index];      break;
    case vHidd_Controller_Ctl_Hat:      if (msg->index < dev->nhats)      d = &dev->hats[msg->index];      break;
    case vHidd_Controller_Ctl_Touchpad: if (msg->index < dev->ntouchpads) d = &dev->touchpads[msg->index]; break;
    case vHidd_Controller_Ctl_Sensor:   if (msg->index < dev->nsensors)   d = &dev->sensors[msg->index];   break;
    }
    if (!d)
        return FALSE;
    *msg->dst = *d;
    if (msg->kind == vHidd_Controller_Ctl_Axis)
    {
        msg->dst->flat = dev->axis_flat[msg->index];
        msg->dst->fuzz = dev->axis_fuzz[msg->index];
    }
    return TRUE;
}

BOOL Controller__Hidd_Controller__GetOutputInfo(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetOutputInfo *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev || !msg->dst || msg->index >= dev->noutputs)
        return FALSE;
    *msg->dst = dev->outputs[msg->index];
    return TRUE;
}

BOOL Controller__Hidd_Controller__GetBinding(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetBinding *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);
    LONG slot = -1;

    if (!dev || !msg->dst || !(dev->flags & CTRL_DF_MAPPED))
        return FALSE;

    if (vHidd_Controller_Std_IsButton(msg->stdid) && msg->stdid < vHidd_Controller_GP_ButtonCount)
        slot = msg->stdid;
    else if (vHidd_Controller_Std_IsAxis(msg->stdid) && (msg->stdid & 0xFF) < vHidd_Controller_GPA_AxisCount)
        slot = vHidd_Controller_GP_ButtonCount + (msg->stdid & 0xFF);
    else if (vHidd_Controller_Std_IsArch(msg->stdid) && (msg->stdid & 0xFF) < vHidd_Controller_ARCH_AxisCount)
        slot = vHidd_Controller_GP_ButtonCount + vHidd_Controller_GPA_AxisCount + (msg->stdid & 0xFF);
    if (slot < 0 || !(dev->bind[slot].flags & vHidd_Controller_BF_Valid))
        return FALSE;
    *msg->dst = dev->bind[slot];
    return TRUE;
}

ULONG Controller__Hidd_Controller__GetBindings(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetBindings *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev || !msg->buffer || !(dev->flags & CTRL_DF_MAPPED))
        return 0;
    return ctrl_CopyBindings(dev, msg->buffer, msg->max);
}

UWORD Controller__Hidd_Controller__GetLabel(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetLabel *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev)
        return vHidd_Controller_Label_None;
    return ctrl_LabelFor(dev, msg->stdid);
}

BOOL Controller__Hidd_Controller__GetPowerInfo(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetPowerInfo *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev || !msg->dst)
        return FALSE;
    Disable();
    *msg->dst = dev->power;
    Enable();
    return (dev->caps & vHidd_Controller_Cap_Battery) ? TRUE : FALSE;
}

ULONG Controller__Hidd_Controller__GetSensorData(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetSensorData *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);
    ULONG n = 0, i, cnt, head;

    if (!dev || !msg->dst || msg->sensor >= dev->nsensors)
        return 0;

    Disable();
    cnt = dev->sensor_count[msg->sensor];
    head = dev->sensor_head[msg->sensor];
    /* oldest first */
    for (i = 0; i < cnt && n < msg->max; i++)
    {
        ULONG idx = (head + HIDD_CONTROLLER_SENSOR_RING - cnt + i) % HIDD_CONTROLLER_SENSOR_RING;
        msg->dst[n++] = dev->sensor_ring[msg->sensor][idx];
    }
    dev->sensor_count[msg->sensor] = 0;
    Enable();
    return n;
}

BOOL Controller__Hidd_Controller__GetTouchpadFinger(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetTouchpadFinger *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev || !msg->dst || msg->pad >= dev->ntouchpads || msg->finger >= HIDD_CONTROLLER_MAX_FINGERS)
        return FALSE;
    Disable();
    *msg->dst = dev->fingers[msg->pad][msg->finger];
    Enable();
    return TRUE;
}

BOOL Controller__Hidd_Controller__GetEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_GetEvent *msg)
{
    struct ControllerConsumer *con = ctrl_Consumer(cl, o);

    if (!con || !msg->dst)
        return FALSE;
    return ctrl_ConsumerGetEvent(cl, con, msg->dst);
}

/*****************************************************************************************
    IID_Hidd_Controller - output side. The base class records the request and
    reports whether the device advertises the capability; drivers override and
    call the superclass method after driving the hardware.
*****************************************************************************************/

BOOL Controller__Hidd_Controller__SetRumble(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetRumble *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev || !msg->params)
        return FALSE;
    dev->rumble = *msg->params;
    return (dev->caps & (vHidd_Controller_Cap_Rumble | vHidd_Controller_Cap_TriggerRumble)) ? TRUE : FALSE;
}

BOOL Controller__Hidd_Controller__SetLED(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetLED *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev)
        return FALSE;
    dev->led[0] = msg->red;
    dev->led[1] = msg->green;
    dev->led[2] = msg->blue;
    return (dev->caps & (vHidd_Controller_Cap_LEDRGB | vHidd_Controller_Cap_LEDMono)) ? TRUE : FALSE;
}

BOOL Controller__Hidd_Controller__SetPlayerIndex(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetPlayerIndex *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev)
        return FALSE;
    dev->player_index = msg->index;
    return (dev->caps & vHidd_Controller_Cap_LEDPlayer) ? TRUE : FALSE;
}

BOOL Controller__Hidd_Controller__SendEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SendEffect *msg)
{
    return FALSE;
}

BOOL Controller__Hidd_Controller__SetSensorsEnabled(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetSensorsEnabled *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev)
        return FALSE;
    dev->sensors_enabled = msg->enable;
    return (dev->caps & vHidd_Controller_Cap_Sensors) ? TRUE : FALSE;
}

LONG Controller__Hidd_Controller__UploadEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_UploadEffect *msg)
{
    return -1;
}

BOOL Controller__Hidd_Controller__UpdateEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_UpdateEffect *msg)
{
    return FALSE;
}

BOOL Controller__Hidd_Controller__PlayEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_PlayEffect *msg)
{
    return FALSE;
}

BOOL Controller__Hidd_Controller__StopEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_StopEffect *msg)
{
    return FALSE;
}

BOOL Controller__Hidd_Controller__RemoveEffect(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_RemoveEffect *msg)
{
    return FALSE;
}

BOOL Controller__Hidd_Controller__SetGain(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetGain *msg)
{
    return FALSE;
}

BOOL Controller__Hidd_Controller__SetAutocenter(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetAutocenter *msg)
{
    return FALSE;
}

/*****************************************************************************************
    Driver side methods - called by drivers on their own device object
*****************************************************************************************/

void Controller__Hidd_Controller__PushValue(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_PushValue *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (dev && (dev->flags & CTRL_DF_REGISTERED))
        ctrl_PushValue(cl, dev, msg->kind, msg->index, msg->value, msg->timestamp);
}

void Controller__Hidd_Controller__PushReport(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_PushReport *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (dev && msg->report && (dev->flags & CTRL_DF_REGISTERED))
        ctrl_PushReport(cl, dev, msg->report);
}

void Controller__Hidd_Controller__PushTouch(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_PushTouch *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);
    struct pHidd_Controller_Finger *f;
    UWORD type;

    if (!dev || !msg->state || msg->pad >= dev->ntouchpads || msg->finger >= HIDD_CONTROLLER_MAX_FINGERS)
        return;
    if (!(dev->flags & CTRL_DF_REGISTERED))
        return;

    f = &dev->fingers[msg->pad][msg->finger];
    Disable();
    if (msg->state->down && !f->down)       type = vHidd_Controller_TouchDown;
    else if (!msg->state->down && f->down)  type = vHidd_Controller_TouchUp;
    else if (msg->state->down)              type = vHidd_Controller_TouchMotion;
    else                                    type = 0xFFFF;
    *f = *msg->state;
    Enable();

    if (type != 0xFFFF)
    {
        struct pHidd_Controller_Event ev;

        ev.type = type;
        ev.device_id = dev->id;
        ev.code = (msg->pad << 8) | msg->finger;
        ev.std = vHidd_Controller_Std_None;
        ev.flags = 0;
        ev.reserved = 0;
        ev.value = ((LONG)msg->state->x << 16) | msg->state->y;
        ev.sequence = dev->rd.sequence;
        ev.timestamp = ctrl_Now(cl);
        ctrl_DeliverEvent(cl, dev->hw, dev, &ev);
    }
}

void Controller__Hidd_Controller__PushSensor(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_PushSensor *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);
    ULONG i;

    if (!dev || !msg->samples || msg->sensor >= dev->nsensors || !(dev->flags & CTRL_DF_REGISTERED))
        return;

    Disable();
    for (i = 0; i < msg->count; i++)
    {
        UBYTE h = dev->sensor_head[msg->sensor];
        dev->sensor_ring[msg->sensor][h] = msg->samples[i];
        if (!dev->sensor_ring[msg->sensor][h].timestamp)
            dev->sensor_ring[msg->sensor][h].timestamp = ctrl_Now(cl);
        dev->sensor_head[msg->sensor] = (h + 1) % HIDD_CONTROLLER_SENSOR_RING;
        if (dev->sensor_count[msg->sensor] < HIDD_CONTROLLER_SENSOR_RING)
            dev->sensor_count[msg->sensor]++;
    }
    Enable();

    if (msg->count)
    {
        struct pHidd_Controller_Event ev;

        ev.type = vHidd_Controller_SensorUpdate;
        ev.device_id = dev->id;
        ev.code = msg->sensor;
        ev.std = vHidd_Controller_Std_None;
        ev.flags = 0;
        ev.reserved = 0;
        ev.value = msg->count;
        ev.sequence = dev->rd.sequence;
        ev.timestamp = msg->samples[msg->count - 1].timestamp;
        ctrl_DeliverEvent(cl, dev->hw, dev, &ev);
    }
}

void Controller__Hidd_Controller__PushPower(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_PushPower *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);
    BOOL changed;

    if (!dev || !msg->power)
        return;

    Disable();
    changed = dev->power.state != msg->power->state || dev->power.percent != msg->power->percent ||
              dev->power.level != msg->power->level;
    dev->power = *msg->power;
    dev->caps |= vHidd_Controller_Cap_Battery;
    Enable();

    if (changed && (dev->flags & CTRL_DF_REGISTERED))
        ctrl_SendDeviceEvent(cl, dev->hw, dev, vHidd_Controller_PowerChanged, msg->power->percent);
}

void Controller__Hidd_Controller__SetConnected(OOP_Class *cl, OOP_Object *o, struct pHidd_Controller_SetConnected *msg)
{
    struct ControllerDevice *dev = ctrl_Device(cl, o);

    if (!dev)
        return;
    if (dev->connected != (msg->connected ? TRUE : FALSE))
    {
        dev->connected = msg->connected ? TRUE : FALSE;
        Disable();
        if (dev->connected) dev->rd.flags |= vHidd_Controller_RF_Connected;
        else                dev->rd.flags &= ~vHidd_Controller_RF_Connected;
        Enable();
        if (dev->flags & CTRL_DF_REGISTERED)
            ctrl_SendDeviceEvent(cl, dev->hw, dev, vHidd_Controller_ConnectionChanged, dev->connected);
    }
}

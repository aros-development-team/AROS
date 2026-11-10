/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: the fan class. Drivers supply the values; the telemetry entries,
          their names and their units are the same for all.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include "telemetry_intern.h"

# define base CSD(cl)

static const struct
{
    CONST_STRPTR id;
    ULONG        units;
} fanEntries[vHidd_Fan_ValueCount] =
{
    { "Speed",   vHW_TelemetryUnit_RPM     },
    { "Duty",    vHW_TelemetryUnit_Percent },
    { "Running", vHW_TelemetryUnit_Boolean },
    { "Level",   vHW_TelemetryUnit_Raw     }
};

/* The value shown as entry 'index', or -1 */
static LONG Fan_EntryValue(ULONG values, ULONG index)
{
    ULONG which;

    for (which = 0; which < vHidd_Fan_ValueCount; which++)
    {
        if (!(values & vHidd_FanF(which)))
            continue;
        if (index-- == 0)
            return (LONG)which;
    }
    return -1;
}

OOP_Object *Fan__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HIDDFanData *data = OOP_INST_DATA(cl, o);
        ULONG all = vHidd_FanF(vHidd_Fan_ValueCount) - 1;

        data->fan_Values = (ULONG)GetTagData(aHidd_Fan_Values, 0, msg->attrList) & all;
        data->fan_Writable = (ULONG)GetTagData(aHidd_Fan_Writable, 0, msg->attrList) & data->fan_Values;
    }
    return o;
}

VOID Fan__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HIDDFanData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_EntryCount:
        {
            ULONG which, count = 0;

            for (which = 0; which < vHidd_Fan_ValueCount; which++)
                if (data->fan_Values & vHidd_FanF(which))
                    count++;
            *msg->storage = (IPTR)count;
        }
        return;
    }

    Hidd_Fan_Switch(msg->attrID, idx)
    {
    case aoHidd_Fan_Values:
        *msg->storage = (IPTR)data->fan_Values;
        return;
    case aoHidd_Fan_Writable:
        *msg->storage = (IPTR)data->fan_Writable;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL Fan__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    struct HIDDFanData *data = OOP_INST_DATA(cl, o);
    LONG which = Fan_EntryValue(data->fan_Values, msg->index);
    LONG value = 0, min = 0, max = 0;

    if (which < 0)
        return FALSE;
    if (!HIDD_Fan_GetValue(o, (ULONG)which, &value, &min, &max))
        return FALSE;

    Telemetry_FillEntry(cl, msg->tags, fanEntries[which].id, fanEntries[which].units,
                        min, max, value,
                        (data->fan_Writable & vHidd_FanF(which)) ? FALSE : TRUE);
    return TRUE;
}

BOOL Fan__Hidd_Telemetry__SetEntryValue(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_SetEntryValue *msg)
{
    struct HIDDFanData *data = OOP_INST_DATA(cl, o);
    LONG which = Fan_EntryValue(data->fan_Values, msg->index);

    if (which < 0 || !(data->fan_Writable & vHidd_FanF(which)))
        return FALSE;
    return HIDD_Fan_SetValue(o, (ULONG)which, msg->value);
}

/* The driver's part */
BOOL Fan__Hidd_Fan__GetValue(OOP_Class *cl, OOP_Object *o, struct pHidd_Fan_GetValue *msg)
{
    (void)cl;
    (void)o;
    (void)msg;
    return FALSE;
}

BOOL Fan__Hidd_Fan__SetValue(OOP_Class *cl, OOP_Object *o, struct pHidd_Fan_SetValue *msg)
{
    (void)cl;
    (void)o;
    (void)msg;
    return FALSE;
}

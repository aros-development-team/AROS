/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: the thermal zone class. Drivers supply the temperatures; the
          telemetry entries, their names and their unit are the same for all.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include "telemetry_intern.h"

# define base CSD(cl)

static CONST_STRPTR const thermalIds[vHidd_Thermal_PointCount] =
{
    "Temperature", "Passive", "Hot", "Critical", "Shutdown",
    "Active0", "Active1", "Active2", "Active3", "Active4",
    "Active5", "Active6", "Active7", "Active8", "Active9"
};

/* The point shown as entry 'index', or -1 */
static LONG Thermal_EntryPoint(ULONG points, ULONG index)
{
    ULONG point;

    for (point = 0; point < vHidd_Thermal_PointCount; point++)
    {
        if (!(points & vHidd_ThermalF(point)))
            continue;
        if (index-- == 0)
            return (LONG)point;
    }
    return -1;
}

OOP_Object *Thermal__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HIDDThermalData *data = OOP_INST_DATA(cl, o);

        /* a zone always has its sensor */
        data->thermal_Points = vHidd_ThermalF(vHidd_Thermal_Current) |
            ((ULONG)GetTagData(aHidd_Thermal_Points, 0, msg->attrList)
             & (vHidd_ThermalF(vHidd_Thermal_PointCount) - 1));
    }
    return o;
}

VOID Thermal__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HIDDThermalData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_EntryCount:
        {
            ULONG point, count = 0;

            for (point = 0; point < vHidd_Thermal_PointCount; point++)
                if (data->thermal_Points & vHidd_ThermalF(point))
                    count++;
            *msg->storage = (IPTR)count;
        }
        return;
    }

    Hidd_Thermal_Switch(msg->attrID, idx)
    {
    case aoHidd_Thermal_Points:
        *msg->storage = (IPTR)data->thermal_Points;
        return;
    case aoHidd_Thermal_Temperature:
        {
            LONG temperature = 0;

            HIDD_Thermal_GetTemperature(o, vHidd_Thermal_Current, &temperature);
            *msg->storage = (IPTR)temperature;
        }
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

BOOL Thermal__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    struct HIDDThermalData *data = OOP_INST_DATA(cl, o);
    LONG point = Thermal_EntryPoint(data->thermal_Points, msg->index);
    LONG temperature = 0;

    if (point < 0)
        return FALSE;
    /* a point that cannot be read now is not shown as zero degrees */
    if (!HIDD_Thermal_GetTemperature(o, (ULONG)point, &temperature))
        return FALSE;

    Telemetry_FillEntry(cl, msg->tags, thermalIds[point], vHW_TelemetryUnit_MilliCelsius,
                        0, 0, temperature, TRUE);
    return TRUE;
}

/* The driver's part */
BOOL Thermal__Hidd_Thermal__GetTemperature(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Thermal_GetTemperature *msg)
{
    (void)cl;
    (void)o;
    (void)msg;
    return FALSE;
}

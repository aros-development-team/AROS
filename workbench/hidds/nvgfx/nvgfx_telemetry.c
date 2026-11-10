/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: the card's sensors as telemetry objects: a thermal zone, a fan,
          and the supply (core voltage, board power). Each is created only
          when the card has the sensor, added to the system tree, and
          related to the card (it monitors the PCI device; the driver
          exposes it).
*/

#include "nvgfx_intern.h"

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <hidd/system.h>

#include <nouveau_aros_sensors.h>

/* This file's own attribute bases, obtained when the objects are created */
static OOP_AttrBase sensorHiddAB, sensorTelemetryAB, sensorThermalAB, sensorFanAB;

#undef HiddAttrBase
#define HiddAttrBase    sensorHiddAB
#undef HiddTelemetryAB
#define HiddTelemetryAB sensorTelemetryAB
#undef HiddThermalAB
#define HiddThermalAB   sensorThermalAB
#undef HiddFanAB
#define HiddFanAB       sensorFanAB

static IPTR sensor_call(IPTR what, IPTR value, IPTR a2, IPTR a3)
{
    return (IPTR)(SIPTR)nouveau_sensor((int)what, (long *)value);
}

/* One reading; the driver may go as far as an I2C transfer for it */
static BOOL HIDDNVGfxSensor(int what, LONG *value)
{
    long v = 0;

    if ((int)(SIPTR)HIDDNVGfxDrmCall(sensor_call, (IPTR)what, (IPTR)&v, 0, 0) < 0)
        return FALSE;
    *value = (LONG)v;
    return TRUE;
}

/*** The thermal zone ********************************************************/

static const struct
{
    ULONG point;
    int   sensor;
} thermalPoints[] =
{
    { vHidd_Thermal_Current,  NVSENSOR_TEMP           },
    { vHidd_Thermal_Active0,  NVSENSOR_TEMP_FANBOOST  },
    { vHidd_Thermal_Passive,  NVSENSOR_TEMP_DOWNCLOCK },
    { vHidd_Thermal_Critical, NVSENSOR_TEMP_CRITICAL  },
    { vHidd_Thermal_Shutdown, NVSENSOR_TEMP_SHUTDOWN  }
};
#define NUM_THERMALPOINTS (sizeof(thermalPoints) / sizeof(thermalPoints[0]))

BOOL METHOD(NVGfxThermal, Hidd_Thermal, GetTemperature)
{
    ULONG i;

    for (i = 0; i < NUM_THERMALPOINTS; i++)
    {
        if (thermalPoints[i].point == msg->point)
            return HIDDNVGfxSensor(thermalPoints[i].sensor, msg->temperature);
    }
    return FALSE;
}

/*** The fan *****************************************************************/

BOOL METHOD(NVGfxFan, Hidd_Fan, GetValue)
{
    *msg->min = 0;
    *msg->max = 0;

    switch (msg->which)
    {
    case vHidd_Fan_Speed:
        return HIDDNVGfxSensor(NVSENSOR_FAN_RPM, msg->value);
    case vHidd_Fan_Duty:
        if (!HIDDNVGfxSensor(NVSENSOR_FAN_DUTY, msg->value))
            return FALSE;
        /* the range the card's thermal table allows */
        if (!HIDDNVGfxSensor(NVSENSOR_FAN_DUTY_MIN, msg->min) ||
            !HIDDNVGfxSensor(NVSENSOR_FAN_DUTY_MAX, msg->max))
        {
            *msg->min = 0;
            *msg->max = 100;
        }
        return TRUE;
    }
    return FALSE;
}

/*** The supply **************************************************************/

static const struct
{
    CONST_STRPTR id;
    ULONG        units;
    int          sensor, min, max;      /* -1: none */
} powerEntries[] =
{
    { "Core Voltage", vHW_TelemetryUnit_MilliVolts, NVSENSOR_VOLT,  NVSENSOR_VOLT_MIN, NVSENSOR_VOLT_MAX  },
    { "Board Power",  vHW_TelemetryUnit_MilliWatts, NVSENSOR_POWER, -1,                NVSENSOR_POWER_MAX }
};
#define NUM_POWERENTRIES (sizeof(powerEntries) / sizeof(powerEntries[0]))

/* The entry shown as 'index', or -1 */
static LONG HIDDNVGfxPowerEntry(ULONG entries, ULONG index)
{
    ULONG i;

    for (i = 0; i < NUM_POWERENTRIES; i++)
    {
        if (!(entries & (1UL << i)))
            continue;
        if (index-- == 0)
            return (LONG)i;
    }
    return -1;
}

OOP_Object * METHOD(NVGfxPower, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HIDDNVGfxSensorData *data = OOP_INST_DATA(cl, o);
        LONG value;
        ULONG i;

        for (i = 0; i < NUM_POWERENTRIES; i++)
        {
            if (HIDDNVGfxSensor(powerEntries[i].sensor, &value))
                data->entries |= 1UL << i;
        }
    }
    return o;
}

VOID METHOD(NVGfxPower, Root, Get)
{
    struct HIDDNVGfxSensorData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_EntryCount:
        {
            ULONG i, count = 0;

            for (i = 0; i < NUM_POWERENTRIES; i++)
                if (data->entries & (1UL << i))
                    count++;
            *msg->storage = (IPTR)count;
        }
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(NVGfxPower, Hidd_Telemetry, GetEntryAttribs)
{
    struct HIDDNVGfxSensorData *data = OOP_INST_DATA(cl, o);
    LONG entry = HIDDNVGfxPowerEntry(data->entries, msg->index);
    LONG value = 0, min = 0, max = 0;
    struct TagItem *tstate = msg->tags, *tag;

    if (entry < 0 || !HIDDNVGfxSensor(powerEntries[entry].sensor, &value))
        return FALSE;
    if (powerEntries[entry].min >= 0)
        HIDDNVGfxSensor(powerEntries[entry].min, &min);
    if (powerEntries[entry].max >= 0)
        HIDDNVGfxSensor(powerEntries[entry].max, &max);

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case tHidd_Telemetry_EntryID:
            *(CONST_STRPTR *)tag->ti_Data = powerEntries[entry].id;
            break;
        case tHidd_Telemetry_EntryUnits:
            *(ULONG *)tag->ti_Data = powerEntries[entry].units;
            break;
        case tHidd_Telemetry_EntryMin:
            *(LONG *)tag->ti_Data = min;
            break;
        case tHidd_Telemetry_EntryMax:
            *(LONG *)tag->ti_Data = max;
            break;
        case tHidd_Telemetry_EntryValue:
            *(LONG *)tag->ti_Data = value;
            break;
        case tHidd_Telemetry_EntryReadOnly:
            *(BOOL *)tag->ti_Data = TRUE;
            break;
        }
    }
    return TRUE;
}

/*** Creation ****************************************************************/

/* Add one sensor object to the system tree and relate it to the card */
static VOID HIDDNVGfxAddSensor(OOP_Object *root, OOP_Class *sensorclass, CONST_STRPTR hwname,
                               Tag tag, IPTR data, OOP_Object *gfx, OOP_Object *pcidev)
{
    struct TagItem tags[] =
    {
        { aHidd_Name,         (IPTR)"nvgfx.hidd" },
        { aHidd_HardwareName, (IPTR)hwname       },
        { tag,                data               },
        { TAG_DONE,           0                  }
    };
    OOP_Object *sensor;

    if (!sensorclass || !(sensor = HW_AddDriver(root, sensorclass, tags)))
        return;

    /* its readings are about the card; the display driver publishes it */
    if (pcidev)
        HIDD_AddRelation(sensor, vHidd_Relation_Monitors, pcidev, NULL);
    HIDD_AddRelation(gfx, vHidd_Relation_Exposes, sensor, NULL);
}

VOID HIDDNVGfxTelemetryInit(OOP_Class *cl, OOP_Object *gfx, OOP_Object *pcidev)
{
    struct staticdata *sd = SD(cl);
    OOP_Object *root;
    ULONG points = 0, values = 0, i;
    LONG value;

    sensorHiddAB      = OOP_ObtainAttrBase(IID_Hidd);
    sensorTelemetryAB = OOP_ObtainAttrBase(IID_Hidd_Telemetry);
    sensorThermalAB   = OOP_ObtainAttrBase(IID_Hidd_Thermal);
    sensorFanAB       = OOP_ObtainAttrBase(IID_Hidd_Fan);
    if (!sensorHiddAB || !sensorTelemetryAB || !sensorThermalAB || !sensorFanAB)
        return;

    root = OOP_NewObject(NULL, CLID_Hidd_System, NULL);
    if (!root)
        root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (!root)
        return;

    /* The thermal zone, with the thresholds the card's tables give */
    if (HIDDNVGfxSensor(NVSENSOR_TEMP, &value))
    {
        for (i = 1; i < NUM_THERMALPOINTS; i++)
        {
            if (HIDDNVGfxSensor(thermalPoints[i].sensor, &value))
                points |= vHidd_ThermalF(thermalPoints[i].point);
        }
        HIDDNVGfxAddSensor(root, sd->thermalclass, "GPU Thermal Zone",
                           aHidd_Thermal_Points, points, gfx, pcidev);
    }

    /* The fan: its tachometer and, when the driver controls it, its duty */
    if (HIDDNVGfxSensor(NVSENSOR_FAN_RPM, &value))
        values |= vHidd_FanF(vHidd_Fan_Speed);
    if (HIDDNVGfxSensor(NVSENSOR_FAN_DUTY, &value))
        values |= vHidd_FanF(vHidd_Fan_Duty);
    if (values)
        HIDDNVGfxAddSensor(root, sd->fanclass, "GPU Fan", aHidd_Fan_Values, values, gfx, pcidev);

    /* Core voltage and board power (the class looks for them itself) */
    if (HIDDNVGfxSensor(NVSENSOR_VOLT, &value) || HIDDNVGfxSensor(NVSENSOR_POWER, &value))
        HIDDNVGfxAddSensor(root, sd->powerclass, "GPU Power Supply", TAG_IGNORE, 0, gfx, pcidev);
}

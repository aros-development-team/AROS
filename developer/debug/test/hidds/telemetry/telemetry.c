/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: test of the thermal zone and fan telemetry classes. Two drivers
          are made up here on top of CLID_Hidd_Thermal and CLID_Hidd_Fan,
          with known readings, and what the classes present as telemetry
          entries is checked against them.
*/

#include <dos/dos.h>
#include <hidd/hidd.h>
#include <hidd/telemetry.h>
#include <hidd/thermal.h>
#include <hidd/fan.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <stdio.h>
#include <string.h>

struct Library *OOPBase;
OOP_AttrBase HiddAttrBase;
OOP_AttrBase HiddTelemetryAB;
OOP_AttrBase HiddThermalAB;
OOP_AttrBase HiddFanAB;
OOP_AttrBase MetaAttrBase;

static struct OOP_ABDescr abd[] =
{
    {IID_Hidd,           &HiddAttrBase},
    {IID_Hidd_Telemetry, &HiddTelemetryAB},
    {IID_Hidd_Thermal,   &HiddThermalAB},
    {IID_Hidd_Fan,       &HiddFanAB},
    {IID_Meta,           &MetaAttrBase},
    {NULL, NULL}
};

static int failures;

static void check(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        failures++;
}

/*** The made-up thermal zone: no "hot" point, one that cannot be read ***/

static IPTR testThermal_GetTemperature(OOP_Class *cl, OOP_Object *o, struct pHidd_Thermal_GetTemperature *msg)
{
    switch (msg->point)
    {
    case vHidd_Thermal_Current:  *msg->temperature = 45500;  return TRUE;
    case vHidd_Thermal_Passive:  *msg->temperature = 90000;  return TRUE;
    case vHidd_Thermal_Critical: *msg->temperature = 105000; return TRUE;
    case vHidd_Thermal_Active0:  *msg->temperature = -5250;  return TRUE;
    }
    return FALSE;       /* vHidd_Thermal_Shutdown is declared but fails to read */
}

/*** The made-up fan: a tachometer and a duty that can be set ***/

static LONG fanDuty = 40;

static IPTR testFan_GetValue(OOP_Class *cl, OOP_Object *o, struct pHidd_Fan_GetValue *msg)
{
    switch (msg->which)
    {
    case vHidd_Fan_Speed:
        *msg->value = 1500; *msg->min = 0; *msg->max = 0;
        return TRUE;
    case vHidd_Fan_Duty:
        *msg->value = fanDuty; *msg->min = 30; *msg->max = 100;
        return TRUE;
    }
    return FALSE;
}

static IPTR testFan_SetValue(OOP_Class *cl, OOP_Object *o, struct pHidd_Fan_SetValue *msg)
{
    if (msg->which != vHidd_Fan_Duty || msg->value < 30 || msg->value > 100)
        return FALSE;
    fanDuty = msg->value;
    return TRUE;
}

static OOP_Class *makeClass(CONST_STRPTR superID, CONST_STRPTR interfaceID,
                            struct OOP_MethodDescr *methods, ULONG nummethods)
{
    struct OOP_InterfaceDescr ifdescr[] =
    {
        { methods, (STRPTR)interfaceID, nummethods },
        { NULL, NULL, 0 }
    };
    struct TagItem tags[] =
    {
        { aMeta_SuperID,        (IPTR)superID },
        { aMeta_InterfaceDescr, (IPTR)ifdescr },
        { aMeta_InstSize,       sizeof(IPTR)  },
        { TAG_DONE,             0             }
    };

    return (OOP_Class *)OOP_NewObject(NULL, CLID_HiddMeta, tags);
}

struct Entry
{
    CONST_STRPTR id;
    IPTR units, min, max, value;
    BOOL readOnly;
};

static BOOL getEntry(OOP_Object *o, ULONG index, struct Entry *e)
{
    struct TagItem tags[] =
    {
        { tHidd_Telemetry_EntryID,       (IPTR)&e->id       },
        { tHidd_Telemetry_EntryUnits,    (IPTR)&e->units    },
        { tHidd_Telemetry_EntryMin,      (IPTR)&e->min      },
        { tHidd_Telemetry_EntryMax,      (IPTR)&e->max      },
        { tHidd_Telemetry_EntryValue,    (IPTR)&e->value    },
        { tHidd_Telemetry_EntryReadOnly, (IPTR)&e->readOnly },
        { TAG_DONE, 0 }
    };

    memset(e, 0, sizeof(*e));
    return HIDD_Telemetry_GetEntryAttribs(o, index, tags);
}

static BOOL entryIs(OOP_Object *o, ULONG index, CONST_STRPTR id, ULONG units, LONG value, LONG min, LONG max, BOOL readOnly)
{
    struct Entry e;

    if (!getEntry(o, index, &e))
    {
        printf("      entry %lu: not answered\n", (unsigned long)index);
        return FALSE;
    }
    printf("      entry %lu: %s = %ld (units %lu, %ld..%ld, %s)\n", (unsigned long)index,
           e.id ? (const char *)e.id : "?", (long)(LONG)e.value, (unsigned long)e.units,
           (long)(LONG)e.min, (long)(LONG)e.max, e.readOnly ? "read-only" : "writable");
    return e.id && !strcmp((const char *)e.id, (const char *)id) && (ULONG)e.units == units &&
           (LONG)e.value == value && (LONG)e.min == min && (LONG)e.max == max &&
           (e.readOnly ? TRUE : FALSE) == readOnly;
}

int main(void)
{
    struct OOP_MethodDescr thermalMethods[] =
    {
        { (OOP_MethodFunc)testThermal_GetTemperature, moHidd_Thermal_GetTemperature },
        { NULL, 0 }
    };
    struct OOP_MethodDescr fanMethods[] =
    {
        { (OOP_MethodFunc)testFan_GetValue, moHidd_Fan_GetValue },
        { (OOP_MethodFunc)testFan_SetValue, moHidd_Fan_SetValue },
        { NULL, 0 }
    };
    OOP_Class *thermalClass = NULL, *fanClass = NULL;
    OOP_Object *zone = NULL, *fan = NULL;
    struct Entry e;
    IPTR val;

    if (!(OOPBase = OpenLibrary("oop.library", 0)))
        return RETURN_FAIL;
    if (!OOP_ObtainAttrBases(abd))
    {
        printf("no attribute bases: is telemetry.hidd 50.3 installed?\n");
        CloseLibrary(OOPBase);
        return RETURN_FAIL;
    }

    thermalClass = makeClass(CLID_Hidd_Thermal, IID_Hidd_Thermal, thermalMethods, num_Hidd_Thermal_Methods);
    fanClass = makeClass(CLID_Hidd_Fan, IID_Hidd_Fan, fanMethods, num_Hidd_Fan_Methods);
    check(thermalClass != NULL, "a driver class on CLID_Hidd_Thermal");
    check(fanClass != NULL, "a driver class on CLID_Hidd_Fan");

    if (thermalClass)
    {
        struct TagItem tags[] =
        {
            { aHidd_Thermal_Points, vHidd_ThermalF(vHidd_Thermal_Passive) | vHidd_ThermalF(vHidd_Thermal_Critical) |
                                    vHidd_ThermalF(vHidd_Thermal_Shutdown) | vHidd_ThermalF(vHidd_Thermal_Active0) },
            { TAG_DONE, 0 }
        };

        zone = OOP_NewObject(thermalClass, NULL, tags);
        check(zone != NULL, "thermal zone object");
    }
    if (zone)
    {
        val = 0;
        OOP_GetAttr(zone, aHidd_Telemetry_EntryCount, &val);
        check(val == 5, "the sensor is always there: 4 declared points + it = 5 entries");
        val = 0;
        OOP_GetAttr(zone, aHidd_Thermal_Points, &val);
        check(val & vHidd_ThermalF(vHidd_Thermal_Current), "aHidd_Thermal_Points includes the sensor");
        val = 0;
        OOP_GetAttr(zone, aHidd_Thermal_Temperature, &val);
        check((LONG)val == 45500, "aHidd_Thermal_Temperature is the sensor's reading");

        check(entryIs(zone, 0, "Temperature", vHW_TelemetryUnit_MilliCelsius, 45500, 0, 0, TRUE), "entry 0 is the temperature");
        check(entryIs(zone, 1, "Passive", vHW_TelemetryUnit_MilliCelsius, 90000, 0, 0, TRUE), "entry 1 is the passive point");
        check(entryIs(zone, 2, "Critical", vHW_TelemetryUnit_MilliCelsius, 105000, 0, 0, TRUE), "entry 2 is the critical point (no hot point declared)");
        check(!getEntry(zone, 3, &e), "a point that fails to read is not answered (not shown as 0)");
        check(entryIs(zone, 4, "Active0", vHW_TelemetryUnit_MilliCelsius, -5250, 0, 0, TRUE), "entry 4 is the first active point, below zero");
        check(!getEntry(zone, 5, &e), "no entry past the last");
        check(!HIDD_Telemetry_SetEntryValue(zone, 0, 1), "a temperature cannot be set");
    }

    if (fanClass)
    {
        struct TagItem tags[] =
        {
            { aHidd_Fan_Values,   vHidd_FanF(vHidd_Fan_Speed) | vHidd_FanF(vHidd_Fan_Duty) },
            { aHidd_Fan_Writable, vHidd_FanF(vHidd_Fan_Duty) | vHidd_FanF(vHidd_Fan_Level) },
            { TAG_DONE, 0 }
        };

        fan = OOP_NewObject(fanClass, NULL, tags);
        check(fan != NULL, "fan object");
    }
    if (fan)
    {
        val = 0;
        OOP_GetAttr(fan, aHidd_Telemetry_EntryCount, &val);
        check(val == 2, "2 entries");
        val = 0;
        OOP_GetAttr(fan, aHidd_Fan_Writable, &val);
        check(val == vHidd_FanF(vHidd_Fan_Duty), "a value the fan does not have cannot be writable");

        check(entryIs(fan, 0, "Speed", vHW_TelemetryUnit_RPM, 1500, 0, 0, TRUE), "entry 0 is the speed, read-only");
        check(entryIs(fan, 1, "Duty", vHW_TelemetryUnit_Percent, 40, 30, 100, FALSE), "entry 1 is the duty with its range, writable");
        check(!HIDD_Telemetry_SetEntryValue(fan, 0, 2000), "the speed cannot be set");
        check(HIDD_Telemetry_SetEntryValue(fan, 1, 55), "the duty can be set");
        check(entryIs(fan, 1, "Duty", vHW_TelemetryUnit_Percent, 55, 30, 100, FALSE), "and reads back");
        check(!HIDD_Telemetry_SetEntryValue(fan, 1, 10), "the driver refuses a value outside its range");
        check(!HIDD_Telemetry_SetEntryValue(fan, 2, 1), "no entry past the last");
    }

    if (fan) OOP_DisposeObject(fan);
    if (zone) OOP_DisposeObject(zone);
    if (fanClass) OOP_DisposeObject((OOP_Object *)fanClass);
    if (thermalClass) OOP_DisposeObject((OOP_Object *)thermalClass);
    OOP_ReleaseAttrBases(abd);
    CloseLibrary(OOPBase);

    printf("%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? RETURN_ERROR : RETURN_OK;
}

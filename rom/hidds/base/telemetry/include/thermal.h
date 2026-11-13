#ifndef HIDD_THERMAL_H
#define HIDD_THERMAL_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: a thermal zone: one temperature sensor and its trip points.

    A driver subclasses CLID_Hidd_Thermal, says at creation which points its
    hardware has (aHidd_Thermal_Points) and implements
    moHidd_Thermal_GetTemperature. The class presents them as telemetry
    entries with the same names and unit for every driver.
*/

#include <hidd/telemetry.h>

#define IID_Hidd_Thermal    "hidd.telemetry.thermal"
#define CLID_Hidd_Thermal   IID_Hidd_Thermal

#include <interface/Hidd_Thermal.h>

/* Temperature points. All are in thousandths of a degree Celsius. */
enum
{
    vHidd_Thermal_Current,      /* the sensor's reading */
    vHidd_Thermal_Passive,      /* the device is slowed down from here */
    vHidd_Thermal_Hot,          /* the system should sleep or save state */
    vHidd_Thermal_Critical,     /* the system should shut down */
    vHidd_Thermal_Shutdown,     /* the hardware switches itself off */
    vHidd_Thermal_Active0,      /* active cooling steps up (fan boost), first of ten */
    vHidd_Thermal_Active9 = vHidd_Thermal_Active0 + 9,

    vHidd_Thermal_PointCount
};

/* The bit of a point in aHidd_Thermal_Points */
#define vHidd_ThermalF(point)   (1UL << (point))

#endif /* !HIDD_THERMAL_H */

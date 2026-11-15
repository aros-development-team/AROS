#ifndef HIDD_FAN_H
#define HIDD_FAN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: a fan.

    A driver subclasses CLID_Hidd_Fan, says at creation which values its
    hardware reports (aHidd_Fan_Values) and which of them can be set
    (aHidd_Fan_Writable), and implements moHidd_Fan_GetValue (and
    moHidd_Fan_SetValue). The class presents them as telemetry entries with
    the same names and units for every driver.
*/

#include <hidd/telemetry.h>

#define IID_Hidd_Fan        "hidd.telemetry.fan"
#define CLID_Hidd_Fan       IID_Hidd_Fan

#include <interface/Hidd_Fan.h>

/* Fan values */
enum
{
    vHidd_Fan_Speed,            /* revolutions per minute */
    vHidd_Fan_Duty,             /* drive level, percent */
    vHidd_Fan_Running,          /* boolean */
    vHidd_Fan_Level,            /* the driver's own step or control number */

    vHidd_Fan_ValueCount
};

/* The bit of a value in aHidd_Fan_Values and aHidd_Fan_Writable */
#define vHidd_FanF(value)       (1UL << (value))

#endif /* !HIDD_FAN_H */

#ifndef HIDD_TELEMETRY_H
#define HIDD_TELEMETRY_H

#define IID_Hidd_Telemetry    "hidd.telemetry"
#define CLID_Hidd_Telemetry   IID_Hidd_Telemetry

#include <utility/tagitem.h>

#define tHidd_Telemetry_BASE        TAG_USER
#define tHidd_Telemetry_EntryID     (tHidd_Telemetry_BASE + 0x1000)
#define tHidd_Telemetry_EntryUnits  (tHidd_Telemetry_BASE + 0x1001)
#define tHidd_Telemetry_EntryMin    (tHidd_Telemetry_BASE + 0x1002)
#define tHidd_Telemetry_EntryMax    (tHidd_Telemetry_BASE + 0x1003)
#define tHidd_Telemetry_EntryValue  (tHidd_Telemetry_BASE + 0x1004)
#define tHidd_Telemetry_EntryReadOnly (tHidd_Telemetry_BASE + 0x1005)

#include <interface/Hidd_Telemetry.h>

enum {
    vHW_TelemetryUnit_Unknown,
    vHW_TelemetryUnit_Boolean,
    vHW_TelemetryUnit_Raw,
    vHW_TelemetryUnit_Percent,
    vHW_TelemetryUnit_RPM,
    vHW_TelemetryUnit_Celsius,
    vHW_TelemetryUnit_Volts,
    vHW_TelemetryUnit_Amps,
    vHW_TelemetryUnit_Watts
};

#endif /* !HIDD_TELEMETRY_H */

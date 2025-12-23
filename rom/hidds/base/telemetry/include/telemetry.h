#ifndef HIDD_TELEMETRY_H
#define HIDD_TELEMETRY_H

#define IID_Hidd_Telemetry    "hidd.telemetry"
#define CLID_Hidd_Telemetry   IID_Hidd_Telemetry

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

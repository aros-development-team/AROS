#ifndef HIDDTELEMETRY_INTERN_H
#define HIDDTELEMETRY_INTERN_H

#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/telemetry.h>
#include <hidd/thermal.h>
#include <hidd/fan.h>

struct HIDDTelemetryData
{
    ULONG telemetry_Dummy;
};

struct HIDDThermalData
{
    ULONG thermal_Points;       /* vHidd_ThermalF() bits */
};

struct HIDDFanData
{
    ULONG fan_Values;           /* vHidd_FanF() bits */
    ULONG fan_Writable;
};

struct class_static_data
{
    struct Library              *cs_OOPBase;
    BPTR                        cs_SegList;
    struct Library              *cs_UtilityBase;

    OOP_Class                   *telemetryClass;
    OOP_Class                   *thermalClass;
    OOP_Class                   *fanClass;

    OOP_AttrBase                telemetryAttrBase;
    OOP_AttrBase                thermalAttrBase;
    OOP_AttrBase                fanAttrBase;
    OOP_MethodID                thermalMethodBase;
    OOP_MethodID                fanMethodBase;
};

#undef HiddTelemetryAB
#define HiddTelemetryAB               (base->telemetryAttrBase)
#undef HiddThermalAB
#define HiddThermalAB                 (base->thermalAttrBase)
#undef HiddFanAB
#define HiddFanAB                     (base->fanAttrBase)
#undef HiddThermalBase
#define HiddThermalBase               (base->thermalMethodBase)
#undef HiddFanBase
#define HiddFanBase                   (base->fanMethodBase)

struct HiddTelemetryIntBase
{
    struct Library              hbi_LibNode;

    struct class_static_data    hbi_csd;
};

#define CSD(x) (&((struct HiddTelemetryIntBase *)x->UserData)->hbi_csd)

#define OOPBase                         (base->cs_OOPBase)
#define UtilityBase                     (base->cs_UtilityBase)

/* telemetry_class.c: answer a GetEntryAttribs tag list */
VOID Telemetry_FillEntry(OOP_Class *cl, struct TagItem *tags, CONST_STRPTR id,
                         ULONG units, LONG min, LONG max, LONG value, BOOL readOnly);

#endif /* !HIDDTELEMETRY_INTERN_H */

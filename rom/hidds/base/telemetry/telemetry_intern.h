#ifndef HIDDTELEMETRY_INTERN_H
#define HIDDTELEMETRY_INTERN_H

#include <exec/semaphores.h>
#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/telemetry.h>

struct HIDDTelemetryData
{
    ULONG telemetry_Dummy;
};

struct class_static_data
{
    struct Library              *cs_OOPBase;
    BPTR                        cs_SegList;

    OOP_Class                   *telemetryClass;

    OOP_AttrBase                telemetryAttrBase;
};

#undef HiddTelemetryAB
#define HiddTelemetryAB               (base->telemetryAttrBase)

struct HiddTelemetryIntBase
{
    struct Library              hbi_LibNode;

    struct class_static_data    hbi_csd;
};

#define CSD(x) (&((struct HiddTelemetryIntBase *)x->UserData)->hbi_csd)

#define OOPBase                         (base->cs_OOPBase)

#endif /* !HIDDTELEMETRY_INTERN_H */

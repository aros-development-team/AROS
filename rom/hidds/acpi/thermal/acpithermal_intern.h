#ifndef HWACPITHERMAL_INTERN_H
#define HWACPITHERMAL_INTERN_H

#include <proto/acpica.h>

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/telemetry.h>
#include <hidd/acpithermal.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#define ACPITHERMAL_ACTIVE_MAX 10

enum
{
    ACPITHERMAL_FLAG_CRT = 1 << 0,
    ACPITHERMAL_FLAG_HOT = 1 << 1,
    ACPITHERMAL_FLAG_PSV = 1 << 2
};

struct HWACPIThermalData
{
    ACPI_HANDLE                 acpithermal_Handle;
    ULONG                       acpithermal_TelemetryCount;
    ULONG                       acpithermal_Flags;
    ULONG                       acpithermal_ActiveMask;
};

struct ACPIThermalNode
{
    struct Node                 atzn_Node;
    APTR                        atzn_Handle;
    OOP_Object                  *atzn_Object;
};

struct acpithermalclass_staticdata
{
    struct Library              *cs_OOPBase;
    struct Library              *cs_UtilityBase;
    struct Library              *cs_ACPICABase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;

    struct List                 cs_Thermals;

    OOP_AttrBase                hwAB;
    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hiddTelemetryAB;
    OOP_AttrBase                hwACPIThermalAB;
};

struct HWACPIThermalIntBase
{
    struct Library              hsi_LibNode;

    struct acpithermalclass_staticdata    hsi_csd;
};

#define CSD(x) (&((struct HWACPIThermalIntBase *)x->UserData)->hsi_csd)
#define _csd    CSD(cl)

#define __IHW_ACPIThermal       (_csd->hwACPIThermalAB)
#define __IHW                 (_csd->hwAB)
#define __IHidd               (_csd->hiddAB)
#define __IHidd_Telemetry     (_csd->hiddTelemetryAB)

#define OOPBase                 (_csd->cs_OOPBase)

#endif /* !HWACPITHERMAL_INTERN_H */

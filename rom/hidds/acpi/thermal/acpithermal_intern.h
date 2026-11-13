#ifndef HWACPITHERMAL_INTERN_H
#define HWACPITHERMAL_INTERN_H

#include <proto/acpica.h>

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/telemetry.h>
#include <hidd/thermal.h>
#include <hidd/acpithermal.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

struct HWACPIThermalData
{
    ACPI_HANDLE                 acpithermal_Handle;
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
    OOP_AttrBase                hiddThermalAB;
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
#define __IHidd_Thermal       (_csd->hiddThermalAB)

#define OOPBase                 (_csd->cs_OOPBase)

#endif /* !HWACPITHERMAL_INTERN_H */

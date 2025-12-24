#ifndef HWACPIACAD_INTERN_H
#define HWACPIACAD_INTERN_H

#include <proto/acpica.h>

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/power.h>
#include <hidd/telemetry.h>
#include <hidd/acpiacad.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

struct HWACPIACAdData
{
    ACPI_HANDLE                 acpiacad_Handle;
    ULONG                       acpiacad_State;
    ULONG                       acpiacad_Flags;
    ULONG                       acpiacad_TelemetryCount;
};

struct ACPIACAdNode
{
    struct Node                 aacadn_Node;
    APTR                        aacadn_Handle;
    OOP_Object                  *aacadn_Object;
};

struct acpiacadclass_staticdata
{
    struct Library	            *cs_OOPBase;
    struct Library              *cs_UtilityBase;
    struct Library              *cs_ACPICABase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;

    struct List                 cs_Batteries;

    OOP_AttrBase                hwAB;
    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hiddTelemetryAB;
    OOP_AttrBase                hiddPowerAB;
    OOP_AttrBase                hwACPIACAdAB;
};

/* Library base */

struct HWACPIACAdIntBase
{
    struct Library              hsi_LibNode;

    struct acpiacadclass_staticdata    hsi_csd;
};

#define CSD(x) (&((struct HWACPIACAdIntBase *)x->UserData)->hsi_csd)
#define _csd    CSD(cl)

#define __IHW_ACPIACAd          (_csd->hwACPIACAdAB)
#define __IHW 	                (_csd->hwAB)
#define __IHidd	                (_csd->hiddAB)
#define __IHidd_Telemetry       (_csd->hiddTelemetryAB)
#define __IHidd_Power           (_csd->hiddPowerAB)

#define OOPBase                 (_csd->cs_OOPBase)

#endif /* !HWACPIACAD_INTERN_H */

#ifndef HWACPIFAN_INTERN_H
#define HWACPIFAN_INTERN_H

#include <proto/acpica.h>

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/telemetry.h>
#include <hidd/acpifan.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

struct HWACPIFanData
{
    ACPI_HANDLE                 acpifan_Handle;
    ULONG                       acpifan_TelemetryCount;
    ULONG                       acpifan_FpsMin;
    ULONG                       acpifan_FpsMax;
    BOOL                        acpifan_HasFps;
    BOOL                        acpifan_HasStatus;
    BOOL                        acpifan_HasControl;
};

struct ACPIFanNode
{
    struct Node                 afann_Node;
    APTR                        afann_Handle;
    OOP_Object                  *afann_Object;
};

struct acpifanclass_staticdata
{
    struct Library              *cs_OOPBase;
    struct Library              *cs_UtilityBase;
    struct Library              *cs_ACPICABase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;

    struct List                 cs_Fans;

    OOP_AttrBase                hwAB;
    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hiddTelemetryAB;
    OOP_AttrBase                hwACPIFanAB;
};

struct HWACPIFanIntBase
{
    struct Library              hsi_LibNode;

    struct acpifanclass_staticdata    hsi_csd;
};

#define CSD(x) (&((struct HWACPIFanIntBase *)x->UserData)->hsi_csd)
#define _csd    CSD(cl)

#define __IHW_ACPIFan          (_csd->hwACPIFanAB)
#define __IHW                 (_csd->hwAB)
#define __IHidd               (_csd->hiddAB)
#define __IHidd_Telemetry     (_csd->hiddTelemetryAB)

#define OOPBase                 (_csd->cs_OOPBase)

#endif /* !HWACPIFAN_INTERN_H */

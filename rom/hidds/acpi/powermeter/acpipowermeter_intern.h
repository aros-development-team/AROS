#ifndef HWACPIPOWERMETER_INTERN_H
#define HWACPIPOWERMETER_INTERN_H

#include <proto/acpica.h>

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/telemetry.h>
#include <hidd/acpipowermeter.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#define ACPIPM_MAX_ENTRIES 4

struct ACPIPowerMeterEntry
{
    CONST_STRPTR                 apm_Id;
    CONST_STRPTR                 apm_Method;
    ULONG                        apm_Units;
    ULONG                        apm_Scale;
    ULONG                        apm_PackageIndex;
    BOOL                         apm_UsePackage;
    BOOL                         apm_TempKelvin;
};

struct HWACPIPowerMeterData
{
    ACPI_HANDLE                 acpipm_Handle;
    ULONG                       acpipm_TelemetryCount;
    struct ACPIPowerMeterEntry acpipm_Entries[ACPIPM_MAX_ENTRIES];
};

struct ACPIPowerMeterNode
{
    struct Node                 apmn_Node;
    APTR                        apmn_Handle;
    OOP_Object                  *apmn_Object;
};

struct acpipowermeterclass_staticdata
{
    struct Library              *cs_OOPBase;
    struct Library              *cs_UtilityBase;
    struct Library              *cs_ACPICABase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;

    struct List                 cs_Devices;

    OOP_AttrBase                hwAB;
    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hiddTelemetryAB;
    OOP_AttrBase                hwACPIPowerMeterAB;
};

struct HWACPIPowerMeterIntBase
{
    struct Library              hsi_LibNode;

    struct acpipowermeterclass_staticdata    hsi_csd;
};

#define CSD(x) (&((struct HWACPIPowerMeterIntBase *)x->UserData)->hsi_csd)
#define _csd    CSD(cl)

#define __IHW_ACPIPowerMeter   (_csd->hwACPIPowerMeterAB)
#define __IHW                 (_csd->hwAB)
#define __IHidd               (_csd->hiddAB)
#define __IHidd_Telemetry     (_csd->hiddTelemetryAB)

#define OOPBase                 (_csd->cs_OOPBase)

#endif /* !HWACPIPOWERMETER_INTERN_H */

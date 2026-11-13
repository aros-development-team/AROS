#ifndef HWACPIBATTERY_INTERN_H
#define HWACPIBATTERY_INTERN_H

#include <proto/acpica.h>

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/power.h>
#include <hidd/telemetry.h>
#include <hidd/acpibattery.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

struct HWACPIBatteryData
{
    ACPI_HANDLE                 acpib_Handle;
    ULONG                       acpib_State;
    ULONG                       acpib_Flags;
    ULONG                       acpib_TelemetryCount;

    /* old */
    struct Task                 *acpib_ServiceTask;
    struct Hook                 *acpib_Hook;
    BYTE                        acpib_ServiceShutdown;
    BYTE                        acpib_ServiceSleep;
    BYTE                        acpib_ServiceLid;
};

struct ACPIBatNode
{
    struct Node                 abn_Node;
    APTR                        abn_Handle;
    OOP_Object                  *abn_Object;
};

struct acpibatteryclass_staticdata
{
    struct Library	            *cs_OOPBase;
    struct Library              *cs_UtilityBase;
    struct Library              *cs_ACPICABase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;

    struct List                 cs_Batteries;
/** BEGIN OLD **/
    
    ACPI_HANDLE                 acpiPowerBHandle;
    ULONG                       acpiPowerBType;
    ACPI_HANDLE                 acpiSleepBHandle;
    ULONG                       acpiSleepBType;
    ACPI_HANDLE                 acpibLidBHandle;

    OOP_Object                  *powerBatteryObj;
    OOP_Object                  *sleepBatteryObj;
    OOP_Object                  *lidBatteryObj;

/** END OLD **/

    OOP_AttrBase                hwAB;
    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hiddTelemetryAB;
    OOP_AttrBase                hiddPowerAB;
    OOP_AttrBase                hwACPIBatteryAB;
};

/* Library base */

struct HWACPIBatteryIntBase
{
    struct Library              hsi_LibNode;

    struct acpibatteryclass_staticdata    hsi_csd;
};

#define CSD(x) (&((struct HWACPIBatteryIntBase *)x->UserData)->hsi_csd)
#define _csd    CSD(cl)

#define __IHW_ACPIBattery        (_csd->hwACPIBatteryAB)
#define __IHW 	                (_csd->hwAB)
#define __IHidd	                (_csd->hiddAB)
#define __IHidd_Telemetry       (_csd->hiddTelemetryAB)
#define __IHidd_Power           (_csd->hiddPowerAB)

#define OOPBase                 (_csd->cs_OOPBase)

#endif /* !HWACPIBATTERY_INTERN_H */

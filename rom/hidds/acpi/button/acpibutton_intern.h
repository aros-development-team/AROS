#ifndef HWACPIBUTTON_INTERN_H
#define HWACPIBUTTON_INTERN_H

#include <proto/acpica.h>

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/acpibutton.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

struct HWACPIButtonData
{
    ACPI_HANDLE                 acpib_Handle;
    struct Task                 *acpib_ServiceTask;
    struct Hook                 *acpib_Hook;
    ULONG                       acpib_Type;
    BYTE                        acpib_ServiceShutdown;
    BYTE                        acpib_ServiceSleep;
    BYTE                        acpib_ServiceLid;
};

struct acpibuttonclass_staticdata
{
    struct Library	        *cs_OOPBase;
    struct Library              *cs_UtilityBase;
    struct Library              *cs_ACPICABase;
    BPTR                        cs_SegList;

    OOP_Class                   *oopclass;

    ACPI_HANDLE                 acpiPowerBHandle;
    ULONG                       acpiPowerBType;
    ACPI_HANDLE                 acpiSleepBHandle;
    ULONG                       acpiSleepBType;
    ACPI_HANDLE                 acpibLidBHandle;

    OOP_Object                  *powerButtonObj;
    OOP_Object                  *sleepButtonObj;
    OOP_Object                  *lidButtonObj;

    OOP_AttrBase                hwAB;
    OOP_AttrBase                hiddAB;
    OOP_AttrBase                hwACPIButtonAB;
};

/* Library base */

struct HWACPIButtonIntBase
{
    struct Library              hsi_LibNode;

    struct acpibuttonclass_staticdata    hsi_csd;
};

#define CSD(x) (&((struct HWACPIButtonIntBase *)x->UserData)->hsi_csd)
#define _csd    CSD(cl)

#define __IHW_ACPIButton        (_csd->hwACPIButtonAB)
#define __IHW 	                (_csd->hwAB)
#define __IHidd	                (_csd->hiddAB)

#define OOPBase                 (_csd->cs_OOPBase)

#endif /* !HWACPIBUTTON_INTERN_H */

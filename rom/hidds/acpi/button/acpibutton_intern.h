#ifndef HIDDACPIBUTTON_INTERN_H
#define HIDDACPIBUTTON_INTERN_H

#include <proto/acpica.h>

#include <exec/libraries.h>
#include <dos/bptr.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/acpibutton.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

struct HIDDACPIButtonData
{
    ACPI_HANDLE                 acpib_Handle;
    ULONG                       acpib_Type;
};

struct class_static_data
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
    OOP_AttrBase                hiddACPIButtonAB;
};

/* Library base */

struct HiddACPIButtonIntBase
{
    struct Library              hsi_LibNode;

    struct class_static_data    hsi_csd;
};

#define CSD(x) (&((struct HiddACPIButtonIntBase *)x->UserData)->hsi_csd)

#define __abHidd_ACPIButton     (CSD(cl)->hiddACPIButtonAB)
#define __IHW 	                (CSD(cl)->hwAB)
#define __IHidd	                (CSD(cl)->hiddAB)

#endif /* !HIDDACPIBUTTON_INTERN_H */

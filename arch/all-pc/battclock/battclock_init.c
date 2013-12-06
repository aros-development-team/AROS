#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/acpica.h>
#include <proto/exec.h>

#include "battclock_intern.h"
#include "cmos.h"

/* auto init */
static int BattClock_Init(struct BattClockBase *BattClockBase)
{
    struct Library *ACPICABase;
    InitSemaphore(&BattClockBase->sem);
    BattClockBase->century = CENTURY;	/* Default offset */

    if ((ACPICABase = OpenLibrary("acpica.library",0)))
    {
        ACPI_TABLE_FADT *fadt;
        ACPI_STATUS err;

        err = AcpiGetTable("FACP", 1, (ACPI_TABLE_HEADER **)&fadt);
        if (err == AE_OK) {
            if ((fadt->Header.Length >= offsetof(ACPI_TABLE_FADT, Century)) &&
                fadt->Century)
            {
                D(bug("[BattClock] Got RTC century offset 0x%02X from ACPI\n", fadt->Century));
                BattClockBase->century = fadt->Century;
            }
        }
    }

    return 1;
}

ADD2INITLIB(BattClock_Init, 0)

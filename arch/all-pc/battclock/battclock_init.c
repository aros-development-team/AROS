#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <resources/acpi.h>
#include <proto/acpi.h>
#include <proto/exec.h>

#include "battclock_intern.h"
#include "cmos.h"

/* auto init */
static int BattClock_Init(struct BattClockBase *BattClockBase)
{
    APTR ACPIBase;

    InitSemaphore(&BattClockBase->sem);
    BattClockBase->century = CENTURY;	/* Default offset */

    ACPIBase = OpenResource("acpi.resource");
    if (ACPIBase)
    {
    	struct ACPI_TABLE_TYPE_FADT *fadt = ACPI_FindSDT(ACPI_MAKE_ID('F','A','C','P'));

    	if (fadt && (fadt->header.length >= offsetof(struct ACPI_TABLE_TYPE_FADT, century)) &&
    	    fadt->century)
    	{
    	    D(bug("[BattClock] Got RTC century offset 0x%02X from ACPI\n", fadt->century));
    	    BattClockBase->century = fadt->century;
    	}
    }

    return 1;
}

ADD2INITLIB(BattClock_Init, 0)

/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/acpica.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#include <string.h>
#include <stdio.h>

#include LC_LIBDEFS_FILE

static ACPI_STATUS ACPIFoundCallback(ACPI_HANDLE handle, ULONG nesting_level,
    void *context, void **return_value)
{
    IPTR *found = (IPTR *)return_value;

    D(bug("[i8042:ACPI] %s()\n", __func__));

    *found = *found + 1;

    return AE_OK;
}


static int init_i8042acpi(LIBBASETYPEPTR lh)
{
    struct Library *ACPICABase;

    D(bug("[i8042:ACPI] %s()\n", __func__));

    /*
     * If we have ACPI - check if the PS/2 devices are available.
     */

    ACPICABase = OpenLibrary("acpica.library", 0);
    if (ACPICABase)
    {
        ACPI_STATUS status;
        IPTR devicesfound = 0;

        status = AcpiGetDevices("PNP0303", ACPIFoundCallback, NULL, (void **)&devicesfound);
        if (ACPI_FAILURE(status)) {
            D(bug("[i8042:ACPI] %s: No PNP0303 PS/2 Keyboard found\n", __func__);)
        }
        status = AcpiGetDevices("PNP0F03", ACPIFoundCallback, NULL, (void **)&devicesfound);
        if (ACPI_FAILURE(status)) {
            D(bug("[i8042:ACPI] %s: No PNP0F03 PS/2 Mouse found\n", __func__);)
        }

        if (devicesfound)
        {
            D(bug("[i8042:ACPI] %s: Found %u PS/2 device(s)\n", __func__, devicesfound);)
        }
        CloseLibrary(ACPICABase);
        return (devicesfound != 0) ? TRUE  : FALSE;
    }

    return TRUE;

    ReturnInt("HIDD::Init", ULONG, TRUE);
}

ADD2INITLIB(init_i8042acpi, 0)

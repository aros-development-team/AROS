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
    IPTR  mask = (IPTR)context;

    D(bug("[i8042:ACPI] %s()\n", __func__));

    *found = *found | mask;

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

        status = AcpiGetDevices("PNP0303", ACPIFoundCallback, (APTR)(1 << 0), (void **)&devicesfound);
        if (ACPI_FAILURE(status)) {
            D(bug("[i8042:ACPI] %s: No PNP0303 PS/2 Keyboard found\n", __func__);)
        }
        if (devicesfound & (1 << 0))
        {
            lh->csd.cs_Flags |= PS2F_DISABLEKEYB;
        }

        status = AcpiGetDevices("PNP0F03", ACPIFoundCallback, (APTR)(1 << 1), (void **)&devicesfound);
        if (ACPI_FAILURE(status)) {
            D(bug("[i8042:ACPI] %s: No PNP0F03 PS/2 Mouse found\n", __func__);)
        }
        if (devicesfound & (1 << 1))
        {
            lh->csd.cs_Flags |= PS2F_DISABLEMOUSE;
        }

        if (devicesfound)
        {
            D(bug("[i8042:ACPI] %s: Found %u PS/2 device(s)\n", __func__, devicesfound);)
        }
        CloseLibrary(ACPICABase);
        return (devicesfound != 0) ? TRUE  : FALSE;
    }

    return TRUE;

    ReturnInt("i8042::ACPIInit", int, TRUE);
}

ADD2INITLIB(init_i8042acpi, 20)

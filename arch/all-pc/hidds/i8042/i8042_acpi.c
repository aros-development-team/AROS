/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
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

/* ACPI Hardware IDs for PS/2 devices */
#define ACPI_HID_PS2_KEYBOARD   "PNP0303"
#define ACPI_HID_PS2_MOUSE      "PNP0F03"

/* Bitmask flags for PS/2 device detection */
#define ACPI_PS2DEV_KEYBOARD     (1 << 0)
#define ACPI_PS2DEV_MOUSE        (1 << 1)

static ACPI_STATUS i8042_acpi_devicecallback(ACPI_HANDLE handle, ULONG nesting_level,
        void *context, void **return_value)
{
    IPTR *found = (IPTR *)return_value;
    IPTR  mask = (IPTR)context;

    D(bug("[i8042:ACPI] %s()\n", __func__));

    *found = *found | mask;

    return AE_OK;
}

static int i8042_acpi_probe(LIBBASETYPEPTR lh)
{
    struct Library *ACPICABase;

    D(bug("[i8042:ACPI] %s()\n", __func__));

    /*
     * If ACPI is present, use it to check whether PS/2 keyboard/mouse are enabled.
     */

    ACPICABase = OpenLibrary("acpica.library", 0);
    if (ACPICABase) {
        ACPI_STATUS status;
        IPTR devicesfound = 0;

        status = AcpiGetDevices(ACPI_HID_PS2_KEYBOARD, i8042_acpi_devicecallback, (APTR)ACPI_PS2DEV_KEYBOARD, (void **)&devicesfound);
        if (ACPI_FAILURE(status)) {
            D(bug("[i8042:ACPI] %s: No PNP0303 PS/2 Keyboard found\n", __func__);)
        }
        if (devicesfound & ACPI_PS2DEV_KEYBOARD) {
            lh->csd.cs_Flags |= PS2F_DISABLEKEYB;
        }

        status = AcpiGetDevices(ACPI_HID_PS2_MOUSE, i8042_acpi_devicecallback, (APTR)ACPI_PS2DEV_MOUSE, (void **)&devicesfound);
        if (ACPI_FAILURE(status)) {
            D(bug("[i8042:ACPI] %s: No PNP0F03 PS/2 Mouse found\n", __func__);)
        }
        if (devicesfound & ACPI_PS2DEV_MOUSE) {
            lh->csd.cs_Flags |= PS2F_DISABLEMOUSE;
        }

        if (devicesfound) {
            D(bug("[i8042:ACPI] %s: Found %u PS/2 device(s)\n", __func__, devicesfound);)
        }
        CloseLibrary(ACPICABase);
        return (devicesfound != 0) ? TRUE  : FALSE;
    }

    return TRUE;

    ReturnInt("i8042::ACPIInit", int, TRUE);
}

ADD2INITLIB(i8042_acpi_probe, 20)

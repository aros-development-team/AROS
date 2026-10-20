/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved.
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

/* Standard PNP keyboard and PS/2 pointing-device IDs. AcpiGetDevices
 * also checks compatible IDs, so vendor-specific devices can match a CID. */
static const char * const i8042_keyboard_ids[] = {
    "PNP0300", "PNP0301", "PNP0302", "PNP0303", "PNP0304", "PNP0305",
    "PNP0306", "PNP0309", "PNP030A", "PNP030B", "PNP0320", "PNP0343",
    "PNP0344", "PNP0345", NULL
};
static const char * const i8042_mouse_ids[] = {
    "PNP0F03", "PNP0F0B", "PNP0F0E", "PNP0F12", "PNP0F13",
    "PNP0F19", "PNP0F1C", NULL
};

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

static BOOL i8042_acpi_find_device(struct Library *ACPICABase,
        const char * const *ids, IPTR mask, IPTR *found)
{
    for (; *ids; ids++) {
        ACPI_STATUS status = AcpiGetDevices((char *)*ids, i8042_acpi_devicecallback,
                (APTR)mask, (void **)found);
        if (ACPI_SUCCESS(status) && (*found & mask))
            return TRUE;
    }
    return FALSE;
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
        IPTR devicesfound = 0;

        if (!i8042_acpi_find_device(ACPICABase, i8042_keyboard_ids,
                ACPI_PS2DEV_KEYBOARD, &devicesfound))
            lh->csd.cs_Flags |= PS2F_DISABLEKEYB;
        if (!i8042_acpi_find_device(ACPICABase, i8042_mouse_ids,
                ACPI_PS2DEV_MOUSE, &devicesfound))
            lh->csd.cs_Flags |= PS2F_DISABLEMOUSE;

        D(
            if (devicesfound & ACPI_PS2DEV_KEYBOARD)
                bug("[i8042:ACPI] %s: Found PS/2 keyboard\n", __func__);
            if (devicesfound & ACPI_PS2DEV_MOUSE)
                bug("[i8042:ACPI] %s: Found PS/2 mouse\n", __func__);
        )
        CloseLibrary(ACPICABase);
        return (devicesfound != 0) ? TRUE  : FALSE;
    }

    return TRUE;

    ReturnInt("i8042::ACPIInit", int, TRUE);
}

/*
 * Probe before i8042_Init (priority 10) creates objects and installs IRQs.
 * A failed INITLIB entry unwinds the library, not HW_AddDriver's objects.
 * Running this afterwards could therefore leave live IRQs using a freed base.
 * The disable flags must also be available before either driver is created.
 */
ADD2INITLIB(i8042_acpi_probe, 5)

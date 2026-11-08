/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/acpica.h>

#include <string.h>

/* ACPI tables for the driver's acpi_get_table(), through acpica.library */

struct Library *ACPICABase;

int amdgpu_aros_acpi_get_table(const char *sig, unsigned int instance, void **table)
{
    ACPI_TABLE_HEADER *hdr = NULL;
    char name[5];

    *table = NULL;

    if (!ACPICABase)
        ACPICABase = OpenLibrary("acpica.library", 0);
    if (!ACPICABase)
        return -1;

    strncpy(name, sig, 4);
    name[4] = '\0';

    if (ACPI_FAILURE(AcpiGetTable(name, instance, &hdr)) || !hdr)
        return -1;

    *table = hdr;
    return 0;
}

static int amdgpu_acpi_exit(void)
{
    if (ACPICABase)
    {
        CloseLibrary(ACPICABase);
        ACPICABase = NULL;
    }
    return TRUE;
}

ADD2EXIT(amdgpu_acpi_exit, 5);

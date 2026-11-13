#ifndef KERNEL_ACPI_H
#define KERNEL_ACPI_H
/*
    Copyright © 2017-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS ACPI definitions.
    Lang: english
*/

#include <utility/hooks.h>
#include <acpica/actbl.h>

struct ACPIData {
    struct List                 acpi_tablehooks;
    ULONG                       acpi_apicCnt;
    ULONG                       acpi_ioapicCnt;
    UWORD                       acpi_interruptOverrides;    /* Mask of 0-15 overriden interrupts */

    /* cached pointers to the ACPI Tables */
    CONST_APTR	                acpi_fadt;	        /* FADT pointer			                */
    CONST_APTR	                acpi_madt;	        /* MADT pointer			                */
};

struct ACPI_TABLE_HOOK {
    struct Node                 acpith_Node;
    struct Hook                 acpith_Hook;
    ULONG                       acpith_HeaderLen;
    UINT8                       acpith_EntryType;
    APTR                        acpith_UserData;
};

struct ACPI_TABLESCAN_DATA {
    ACPI_TABLE_HEADER           *acpits_Table;
    APTR                        acpits_UserData;
};

typedef void(acpi_supportinit_t)(struct PlatformData *);
void acpi_Init(struct PlatformData *pdata);
int acpi_ScanTableEntries(CONST ACPI_TABLE_HEADER *, ULONG, UINT8, const struct Hook *, APTR);

#endif /* !KERNEL_ACPI_H */

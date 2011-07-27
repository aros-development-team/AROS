#ifndef HARDWARE_EFI_CONFIG_H
#define HARDWARE_EFI_CONFIG_H

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: EFI firmware configuration tables
    Lang: english
*/

#include <libraries/uuid.h>

struct EFI_Config
{
    uuid_t  VendorGUID;
    void   *Table;
};

/* Known GUIDs */
#define ACPI_20_TABLE_GUID    MAKE_UUID(0X8868E871, 0XE4F1, 0X11D3, 0XBC220080C73C8881)
#define ACPI_TABLE_GUID       MAKE_UUID(0XEB9D2D30, 0X2D88, 0X11D3, 0X9A160090273FC14D)
#define SAL_SYSTEM_TABLE_GUID MAKE_UUID{0XEB9D2D32, 0X2D88, 0X11D3, 0X9A160090273FC14D)
#define SMBIOS_TABLE_GUID     MAKE_UUID(0XEB9D2D31, 0X2D88, 0X11D3, 0X9A160090273FC14D)
#define MPS_TABLE_GUID        MAKE_UUID(0XEB9D2D2F, 0X2D88, 0X11D3, 0X9A160090273FC14D)

#endif

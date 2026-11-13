#ifndef HARDWARE_EFI_TABLES_H
#define HARDWARE_EFI_TABLES_H

/*
    Copyright © 2011-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Basic definitions for EFI tables
    Lang: english
*/

#include <hardware/efi/types.h>

/* Common header of all tables */
struct EFI_TableHeader
{
    UQUAD Signature;
    ULONG Revision;
    ULONG HeaderSize;
    ULONG CRC32;
    ULONG Reserved;
};

#endif

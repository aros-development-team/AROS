#ifndef HARDWARE_SMBIOS_H
#define HARDWARE_SMBIOS_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SMBIOS (DMTF DSP0134) entry point and structure definitions.
*/

#include <exec/types.h>

/* Common structure header. Strings follow the formatted area, each
   NUL terminated, with the set terminated by an extra NUL. */
struct SMBIOSHeader
{
    UBYTE sm_Type;
    UBYTE sm_Length;
    UWORD sm_Handle;
} __attribute__((packed));

/* 32-bit ("_SM_") entry point, SMBIOS 2.x. Length 0x1F. */
struct SMBIOSEntryPoint2
{
    UBYTE anchor[4];                /* "_SM_" */
    UBYTE checksum;                 /* bytes [0, length) sum to zero */
    UBYTE length;
    UBYTE major;
    UBYTE minor;
    UWORD max_structure_size;
    UBYTE entry_point_revision;
    UBYTE formatted_area[5];
    UBYTE intermediate_anchor[5];   /* "_DMI_" */
    UBYTE intermediate_checksum;
    UWORD table_length;             /* exact length of the structure table */
    ULONG table_address;
    UWORD number_of_structures;
    UBYTE bcd_revision;
} __attribute__((packed));

/* 64-bit ("_SM3_") entry point, SMBIOS 3.x. Length 0x18. */
struct SMBIOSEntryPoint3
{
    UBYTE anchor[5];                /* "_SM3_" */
    UBYTE checksum;                 /* bytes [0, length) sum to zero */
    UBYTE length;
    UBYTE major;
    UBYTE minor;
    UBYTE docrev;
    UBYTE entry_point_revision;
    UBYTE reserved;
    ULONG table_length;             /* maximum length of the structure table */
    UQUAD table_address;
} __attribute__((packed));

/* Structure types used by AROS */
#define SMBIOS_TYPE_BIOS            0
#define SMBIOS_TYPE_SYSTEM          1
#define SMBIOS_TYPE_BASEBOARD       2
#define SMBIOS_TYPE_CHASSIS         3
#define SMBIOS_TYPE_PROCESSOR       4
#define SMBIOS_TYPE_IPMI            38
#define SMBIOS_TYPE_END             127

#endif /* HARDWARE_SMBIOS_H */

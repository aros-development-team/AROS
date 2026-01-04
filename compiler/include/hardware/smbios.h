#ifndef HARDWARE_SMBIOS_H
#define HARDWARE_SMBIOS_H

#include <exec/types.h>

struct SMBIOSHeader
{
    UBYTE sm_Type;
    UBYTE sm_Length;
    UWORD sm_Handle;
};

struct SMBIOSEntryPoint2
{
    UBYTE anchor[4];
    UBYTE checksum;
    UBYTE length;
    UBYTE major;
    UBYTE minor;
    UWORD max_structure_size;
    UBYTE entry_point_revision;
    UBYTE formatted_area[5];
    UBYTE intermediate_anchor[5];
    UBYTE intermediate_checksum;
    UWORD table_length;
    ULONG table_address;
    UWORD number_of_structures;
    UBYTE bcd_revision;
};

struct SMBIOSEntryPoint3
{
    UBYTE anchor[5];
    UBYTE checksum;
    UBYTE length;
    UBYTE major;
    UBYTE minor;
    UBYTE docrev;
    UBYTE entry_point_revision;
    UBYTE reserved;
    UQUAD table_address;
    ULONG table_length;
};

#endif /* HARDWARE_SMBIOS_H */

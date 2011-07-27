#ifndef HARDWARE_EFI_TABLES_H
#define HARDWARE_EFI_TABLES_H

#include <exec/types.h>

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

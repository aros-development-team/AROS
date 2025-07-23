#ifndef HARDWARE_EFI_RUNTIME_H
#define HARDWARE_EFI_RUNTIME_H

/*
    Copyright © 2011-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EFI firmware runtime services
    Lang: english
*/

#include <hardware/efi/tables.h>
#include <libraries/uuid.h>

/* Time value */
struct EFI_Time
{
    UWORD Year;
    UBYTE Month;
    UBYTE Day;
    UBYTE Hour;
    UBYTE Minute;
    UBYTE Second;
    UBYTE Pad1;
    ULONG Nano;
    WORD  TimeZone;
    UBYTE Daylight;	/* Flags, see below */
    UBYTE Pad2;
};

/* Tells that clock are running in local time */
#define EFI_TIMEZONE_LOCAL 0x07FF

/* Daylight flags */
#define EFI_TIME_ADJUST_DST 0x01
#define EFI_TIME_IN_DST	    0x02

/* Time capabilities */
struct EFI_Time_Caps
{
    ULONG Resolution;
    ULONG Accuracy;
    UBYTE SetsToZero;
};

/* Reset types */
enum
{
    EFI_Reset_Cold,
    EFI_Reset_Warm,
    EFI_Reset_Shutdown
};

struct EFI_RuntimeServices {
    struct EFI_TableHeader Hdr;

    // Time Services
    __eficall EFI_STATUS (*GetTime)(
        struct EFI_Time *Time,
        struct EFI_Time_Cap *Capabilities
    );
    __eficall EFI_STATUS (*SetTime)(
        struct EFI_Time *Time
    );
    __eficall EFI_STATUS (*GetWakeupTime)(
        UBYTE *Enabled,
        UBYTE *Pending,
        struct EFI_Time *Time
    );
    __eficall EFI_STATUS (*SetWakeupTime)(
        UBYTE Enable,
        struct EFI_Time *Time
    );

    // Virtual Memory Services
    __eficall EFI_STATUS (*SetVirtualAddressMap)(
        UQUAD MemoryMapSize,
        UQUAD DescriptorSize,
        ULONG DescriptorVersion,
        EFI_MEMORY_DESCRIPTOR *VirtualMap
    );
    __eficall EFI_STATUS (*ConvertPointer)(
        UQUAD DebugDisposition,
        void **Address
    );

    // Variable Services
    __eficall EFI_STATUS (*GetVariable)(
        const CHAR16 *VariableName,
        const EFI_GUID *VendorGuid,
        ULONG *Attributes,
        UQUAD *DataSize,
        void *Data
    );
    __eficall EFI_STATUS (*GetNextVariableName)(
        UQUAD *VariableNameSize,
        const CHAR16 *VariableName,
        const EFI_GUID *VendorGuid
    );
    __eficall EFI_STATUS (*SetVariable)(
        const CHAR16 *VariableName,
        const EFI_GUID *VendorGuid,
        ULONG Attributes,
        UQUAD DataSize,
        void *Data
    );

    // Misc Services
    __eficall EFI_STATUS (*GetNextHighMonotonicCount)(
        UQUAD *HighCount
    );
    __eficall void (*ResetSystem)(
        ULONG ResetType,
        EFI_STATUS ResetStatus,
        UQUAD DataSize,
        const CHAR16 *ResetData
    );
};

#define EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552

#endif

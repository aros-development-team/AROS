#ifndef HARDWARE_EFI_RUNTIME_H
#define HARDWARE_EFI_RUNTIME_H

/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
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

struct EFI_Runtime
{
    struct EFI_TableHeader Hdr;

    __eficall SIPTR (*GetTime)(struct EFI_Time *Time, struct EFI_Time_Cap *Caps);
    __eficall SIPTR (*SetTime)(struct EFI_Time *Time);
    __eficall SIPTR (*GetWakeupTime)(UBYTE *Enabled, UBYTE *Pending, struct EFI_Time *Time);
    __eficall SIPTR (*SetWakeupTime)(UBYTE Enabled, struct EFI_Time *Time);
    __eficall SIPTR (*SetVirtualAddressMap)(IPTR MapSize, IPTR EntrySize, ULONG EntryVersion, struct EFI_MemMap *Map);
    __eficall SIPTR (*ConvertPointer)(IPTR DebugDisposition, void **Addr);
    __eficall SIPTR (*GetVariable)(UWORD *Name, uuid_t *VendorGUID, ULONG *Attrs, IPTR *DataSize, void *Data);
    __eficall SIPTR (*GetNextVariableName)(IPTR *NameSize, UWORD *Name, uuid_t *VendorGUID);
    __eficall SIPTR (*SetVariable)(UWORD *Name, uuid_t *VendorGUID, ULONG Attrs, void *Data);
    __eficall SIPTR (*GetNextHighMonotonicCount)(ULONG *HighCount);
    __eficall void  (*ResetSystem)(ULONG Type, SIPTR Status, IPTR DataSize, UWORD *Data);
};

#define EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552

#endif

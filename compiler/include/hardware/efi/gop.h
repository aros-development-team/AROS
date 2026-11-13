#ifndef HARDWARE_EFI_GOP_H
#define HARDWARE_EFI_GOP_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: EFI firmware GOP definitions
    Lang: english
*/

#include <hardware/efi/types.h>

typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

#define PixelRedGreenBlueReserved8BitPerColor   0
#define PixelBlueGreenRedReserved8BitPerColor   1
#define PixelBitMask                            2
#define PixelBltOnly                            3
#define PixelFormatMax                          4

struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION {
    ULONG Version;
    ULONG HorizontalResolution;
    ULONG VerticalResolution;
    ULONG PixelFormat;
    union {
        struct {
            UBYTE RedMask;
            UBYTE GreenMask;
            UBYTE BlueMask;
            UBYTE ReservedMask;
        } PixelInformation;
        UQUAD Raw; // raw 64-bit fallback if needed
    } PixelInfo;
    ULONG PixelsPerScanLine;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE {
    ULONG MaxMode;
    ULONG Mode;
    struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UQUAD SizeOfInfo;
    UQUAD FrameBufferBase;
    UQUAD FrameBufferSize;
};

typedef SIPTR (__eficall *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    ULONG ModeNumber,
    ULONG *SizeOfInfo,
    struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
);

typedef SIPTR (__eficall *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    ULONG ModeNumber
);

typedef SIPTR (__eficall *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    void *BltBuffer,
    ULONG BltOperation,
    ULONG SourceX,
    ULONG SourceY,
    ULONG DestinationX,
    ULONG DestinationY,
    ULONG Width,
    ULONG Height,
    ULONG Delta
);

struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE   SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT        Blt;
    struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
};

#endif

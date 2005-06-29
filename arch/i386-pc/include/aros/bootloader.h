#ifndef AROS_BOOTLOADER_H
#define AROS_BOOTLOADER_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: bootloader.resource general defines
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif 
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/* Requestable information */
#define BL_BASE (TAG_USER)

#define BL_MemoryMap	(BL_BASE + 1)
#define BL_Memory	(BL_BASE + 2)
#define BL_BootDev	(BL_BASE + 3)
#define BL_Args		(BL_BASE + 4)
#define BL_Modules	(BL_BASE + 5)
#define BL_Drives	(BL_BASE + 6)
#define BL_LoaderName	(BL_BASE + 7)
#define BL_Video	(BL_BASE + 8)

/* Structures */
struct MemMapNode {
    struct MinNode node;
    ULONG Base;
    ULONG Length;
    ULONG Type;
};

/* Defines for MemMapNode type field */
#define MMAP_Type_Unknown	0
#define	MMAP_Type_RAM		1
#define MMAP_Type_Reserved	2
#define MMAP_Type_ACPIData	3
#define MMAP_Type_ACPINVS	4

struct DriveInfoNode {
    struct Node node;
    UBYTE	Number;
    UBYTE	Mode;
    UWORD	Cylinders;
    UBYTE	Heads;
    UBYTE	Sectors;
};

#define DriveMode_CHS		0
#define DriveMode_LBA		1

struct VesaInfo {
    APTR	FrameBuffer;
    ULONG	FrameBufferSize; /* In KBytes! */
    ULONG	XSize;
    ULONG	YSize;
    ULONG	BytesPerLine;
    UWORD	BitsPerPixel;
    UWORD	ModeNumber;
    ULONG	Masks[4];
    ULONG	Shifts[4];
};

#define	VI_Red		0
#define	VI_Blue		1
#define	VI_Green	2
#define VI_Alpha	3

#endif /* AROS_BOOTLOADER_H */

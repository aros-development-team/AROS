#ifndef DOS_FILEHANDLER_H
#define DOS_FILEHANDLER_H

/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Constants for filehandlers
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef DOS_BPTR_H
#include <dos/bptr.h>
#endif

#define DE_TABLESIZE    0       /* Must be at least 11 */
#define DE_SIZEBLOCK    1       /* LONGs per block. */
#define DE_BLOCKSIZE    2       /* Bytes per block. */
#define DE_NUMHEADS     3       /* Number of heads per cylinder */
#define DE_SECSPERBLOCK 4       /* not used, must be 1 */
#define DE_BLKSPERTRACK 5       /* Number of blocks per track */
#define DE_RESERVEDBLKS 6       /* unavail. blocks at start (usually 2) */
#define DE_PREFAC       7       /* not used; must be 0 */
#define DE_INTERLEAVE   8       /* usually 0 */
#define DE_LOWCYL       9       /* First cylinder of partition. */
#define DE_HIGHCYL      10      /* Last cylinder of partition. */
#define DE_UPPERCYL     10      /* Alias. */
#define DE_NUMBUFFERS   11      /* Initial Number of disk buffers */
#define DE_BUFMEMTYPE   12      /* Type of memory for device - */
#define DE_MEMBUFTYPE   12      /* - 1 is public, 3 is chip, 5 is fast */
#define DE_MAXTRANSFER  13      /* Max no. of bytes to transfer at a time */
#define DE_MASK         14      /* Address mask for DMA devices */
#define DE_BOOTPRI      15      /* Boot Priority */
#define DE_DOSTYPE      16      /* ASCII (HEX) string of filesystem type */
#define DE_BAUD         17      /* baud rate for serial handler */
#define DE_CONTROL      18      /* control word for handler */
#define DE_BOOTBLOCKS   19      /* number of blocks for boot block */

/* A structure for the above */
struct DosEnvec
{
    ULONG   de_TableSize;
    ULONG   de_SizeBlock;
    ULONG   de_SegOrg;
    ULONG   de_Surfaces;
    ULONG   de_SectorPerBlock;
    ULONG   de_BlocksPerTrack;
    ULONG   de_Reserved;
    ULONG   de_PreAlloc;
    ULONG   de_Interleave;
    ULONG   de_LowCyl;
    ULONG   de_HighCyl;
    ULONG   de_NumBuffers;
    ULONG   de_BufMemTypes;
    ULONG   de_MaxTransfer;
    ULONG   de_Mask;
    LONG    de_BootPri;
    ULONG   de_DosType;
    ULONG   de_Baud;
    ULONG   de_Control;
    ULONG   de_BootBlocks;
};

/* This is the message that is passed to a file handler during startup
   in the DosList->dl_Startup field. It is not used in AROS DOS handlers
   as they are now Device based, and the information is passed in during
   OpenDevice(), however this needs to be stored for late opening
   handlers.
*/

struct FileSysStartupMsg
{
    ULONG   fssm_Unit;      /* exec unit number for this device */
    BSTR    fssm_Device;    /* null term. BSTR to device name */
    BPTR    fssm_Environ;   /* Pointer to DosEnvec table */
    ULONG   fssm_Flags;     /* flags for OpenDevice() */
};

/*  This is an unwound version of the DosList structure defined in the
    dos/dosextens.h include file. This is the version for a DOS "device"
    DLT_DEVICE.

    For AROS this is notable different, as filehandlers are no longer
    DOS tasks (ie Processes), some of the fields here have no purpose
    and are ignored. The only fields retained are the dn_Next, dn_Type,
    dn_Startup and dn_Handler fields.

    Note that the string pointed to by the dn_Handler MUST be NULL
    terminated.
*/

struct DeviceNode
{
    BPTR        dn_Next;        /* singly linked list */
    ULONG       dn_Type;        /* always DLT_DEVICE */
    ULONG       dn_pad1[2];
    BSTR        dn_Handler;     /* BSTR to device name for handler */
    ULONG       dn_pad2[2];
    BPTR        dn_Startup;     /* BPTR to FileSysStartupMsg */
    ULONG       dn_pad3[2];

    BPTR        dn_OldName;     /* BPTR to name of device */
    STRPTR      dn_NewName;     /* STRPTR to same string above */

    struct Device *dn_Device;
    struct Unit   *dn_Unit;
};

#endif /* DOS_FILEHANDLER_H */

#ifndef DOS_FILEHANDLER_H
#define DOS_FILEHANDLER_H

/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Constants for filehandlers.
*/

#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif


/* Disk environment array. The size of this structure is variable.
   de_TableSize contains the size of the structure. This structure may
   look different for different handlers. Most of the entries are in fact
   ULONGs or LONGs, but because every entry must have the same size, we have
   to use IPTRs instead. */
struct DosEnvec
{
    IPTR de_TableSize;      /* Size of this structure. Must be at least
                               11 (DE_NUMBUFFERS). */
    IPTR de_SizeBlock;      /* Size in longwords of a block on the disk. */
    IPTR de_SecOrg;         /* Unused. Must be 0 for now. */
    IPTR de_Surfaces;       /* Number of heads/surfaces in drive. */
    IPTR de_SectorPerBlock; /* Unused. Must be 1 for now. */
    IPTR de_BlocksPerTrack; /* Number of blocks on a track. */
    IPTR de_Reserved;       /* Number of reserved blocks at beginning of
                               volume. */
    IPTR de_PreAlloc;       /* Number of reserved blocks at end of volume. */
    IPTR de_Interleave;
    IPTR de_LowCyl;         /* First cylinder. */
    IPTR de_HighCyl;        /* Last cylinder. */
    IPTR de_NumBuffers;     /* Number of buffers for drive. */
    IPTR de_BufMemType;     /* Type of memory for buffers. See <exec/memory.h>.
                            */
    IPTR de_MaxTransfer;    /* How many bytes may be transferred together? */
    IPTR de_Mask;           /* Memory address mask for DMA devices. */
    IPTR de_BootPri;        /* Priority of Autoboot. */
    IPTR de_DosType;        /* Type of disk. See <dos/dos.h> for definitions.
                            */
    IPTR de_Baud;           /* Baud rate to use. */
    IPTR de_Control;        /* Control word. */
    IPTR de_BootBlocks;     /* Size of bootblock. */
};

/* The following constants are longword offsets, which point into a filehandler
   structure (like the one above). For more information about the meaning
   of these constants see the structure above. */
#define DE_TABLESIZE    0
#define DE_SIZEBLOCK    1
#define DE_BLOCKSIZE    2
#define DE_NUMHEADS     3
#define DE_SECSPERBLOCK 4
#define DE_BLKSPERTRACK 5
#define DE_RESERVEDBLKS 6
#define DE_PREFAC       7
#define DE_INTERLEAVE   8
#define DE_LOWCYL       9
#define DE_HIGHCYL      10
#define DE_UPPERCYL     DE_HIGHCYL
#define DE_NUMBUFFERS   11
#define DE_BUFMEMTYPE   12
#define DE_MEMBUFTYPE   DE_BUFMEMTYPE
#define DE_MAXTRANSFER  13
#define DE_MASK         14
#define DE_BOOTPRI      15
#define DE_DOSTYPE      16
#define DE_BAUD         17
#define DE_CONTROL      18
#define DE_BOOTBLOCKS   19


/* This is the message that is passed to a file handler during startup
   in the DeviceNode->dn_Startup field. It is not used in AROS DOS handlers
   as they are now Device based, and the information is passed in during
   OpenDevice(), however this needs to be stored for late opening
   handlers. */
struct FileSysStartupMsg
{
    ULONG fssm_Unit;    /* Unit number of device used. */
    BSTR  fssm_Device;  /* Device name. */
    BPTR  fssm_Environ; /* Pointer to disk environment array, like the one
                           above. */
    ULONG fssm_Flags;   /* Flags to be passed to OpenDevice(). */
};


/*  This is an unwound version of the DosList structure defined in
    <dos/dosextens.h>. This is the version for a DOS "device" DLT_DEVICE.
    It is essentially the same structure as DevInfo, defined in
    <dos/dosextens.h>.

    For AROS this is notable different, as filehandlers are no longer
    DOS tasks (ie Processes), some of the fields here have no purpose
    and are ignored. The only fields retained are the dn_Next, dn_Type,
    dn_Startup and dn_Handler fields. */
struct DeviceNode
{
      /* PRIVATE pointer to next entry. In AmigaOS this used to be a BPTR. */
    struct DosList * dn_Next;
      /* Type of this node. Has to be DLT_DEVICE. */
    ULONG dn_Type;

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    /* The next two fields are not used by AROS. Their original name was:
       dn_Task and dn_Lock */
    struct MsgPort * dn_NoAROS1;
#endif
    BPTR             dn_Lock;	/* dol_Lock field */

    BSTR dn_Handler;    /* Null-terminated device name for handler. */
    LONG dn_NoAROS2[2]; /* PRIVATE */
    BPTR dn_Startup;    /* (struct FileSysStartupMsg *) see above */
    BPTR dn_NoAROS3[2]; /* PRIVATE */

    /* For the following two fields, see comments in <dos/dosextens.h>.
       Both fields specify the name of the handler. */
    BSTR   dn_OldName;
    STRPTR dn_NewName;

    struct Device *dn_Device;
    struct Unit   *dn_Unit;
};
/* #define dn_Name dn_OldName */

#endif /* DOS_FILEHANDLER_H */

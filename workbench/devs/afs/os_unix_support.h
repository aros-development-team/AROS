#ifndef OS_UNIX_SUPPORT_H
#define OS_UNIX_SUPPORT_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <arpa/inet.h>

/* exec/types.h */
typedef signed char    BYTE;
typedef unsigned char  UBYTE;
typedef signed short   WORD;
typedef unsigned short UWORD;
typedef signed long    LONG;
typedef unsigned long  ULONG;
typedef void*          APTR;
typedef char*          STRPTR;
typedef unsigned long  IPTR;
typedef unsigned long  BPTR;
#define FALSE 0L
#define TRUE  1L

struct AFSBase {};
struct Device {};
struct DeviceList {};
struct DosEnvec {};
struct DriveGeometry {};
struct Interrupt {};
struct List {};
struct Node {};

/* libraries/iffparse.h */
#define MAKE_ID(a,b,c,d) (((ULONG) (a)<<24) | ((ULONG) (b)<<16) | \
                          ((ULONG) (c)<<8)  | ((ULONG) (d)))

/* dos/asl.h */
#define ERROR_BUFFER_OVERFLOW 303

/* dos/dos.h */
#define DOSTRUE (-1)
#define DOSFALSE  0
#define ERROR_UNKNOWN             100
#define ERROR_NO_FREE_STORE       103
#define ERROR_BAD_NUMBER          115
#define ERROR_OBJECT_IN_USE       202
#define ERROR_OBJECT_EXISTS       203
#define ERROR_OBJECT_NOT_FOUND    205
#define ERROR_OBJECT_WRONG_TYPE   212
#define ERROR_DIRECTORY_NOT_EMPTY 216
#define ERROR_SEEK_ERROR          219
#define ERROR_DISK_FULL           221
#define ERROR_DELETE_PROTECTED    222
#define ERROR_NOT_A_DOS_DISK      225
#define ERROR_NO_MORE_ENTRIES     232
#define LOCK_DIFFERENT           -1
#define LOCK_SAME                 0
#define OFFSET_BEGINNING         -1
#define OFFSET_CURRENT            0
#define OFFSET_END                1
#define FIBB_DELETE               0
#define FIBF_DELETE              (1<<FIBB_DELETE)
#define MAXFILENAMELENGTH         108
#define MAXCOMMENTLENGTH           80
struct DateStamp {
	LONG ds_Days;
	LONG ds_Minute;
	LONG ds_Tick;
};
struct FileInfoBlock {
	LONG  fib_DiskKey;
	LONG  fib_DirEntryType;
	UBYTE fib_FileName[MAXFILENAMELENGTH];
	LONG  fib_Protection;
	LONG  fib_EntryType;
	LONG  fib_Size;
	LONG  fib_NumBlocks;
	struct DateStamp fib_Date;
	UBYTE fib_Comment[MAXCOMMENTLENGTH];
	UWORD fib_OwnerUID;
	UWORD fib_OwnerGID;
};
#define FIBB_DELETE   0
#define FIBB_WRITE    2
#define FIBB_READ     3
#define FIBF_DELETE  (1<<FIBB_DELETE)
#define FIBF_WRITE   (1<<FIBB_WRITE)
#define FIBF_READ    (1<<FIBB_READ)
struct InfoData {
	LONG id_NumSoftErrors;
	LONG id_UnitNumber;
	LONG id_DiskState;
	LONG id_NumBlocks;
	LONG id_NumBlocksUsed;
	LONG id_BytesPerBlock;
	LONG id_DiskType;
	BPTR id_VolumeNode;
	LONG id_InUse;
};
#define ID_VALIDATING 81
#define ID_VALIDATED  82
#define ID_DOS_DISK         MAKE_ID('D','O','S',0)
#define ID_FFS_DISK         MAKE_ID('D','O','S',1)
#define ID_INTER_DOS_DISK   MAKE_ID('D','O','S',2)
#define ID_INTER_FFS_DISK   MAKE_ID('D','O','S',3)


/* dos/exall.h */
#define ED_NAME       1
#define ED_TYPE       2
#define ED_SIZE       3
#define ED_PROTECTION 4
#define ED_DATE       5
#define ED_COMMENT    6
#define ED_OWNER      7
struct ExAllData {
	struct ExAllData *ed_Next;
	UBYTE *ed_Name;
	LONG   ed_Type;
	ULONG  ed_Size;
	ULONG  ed_Prot;
	ULONG  ed_Days;
	ULONG  ed_Mins;
	ULONG  ed_Ticks;
	UBYTE *ed_Comment;
	UWORD  ed_OwnerUID;
	UWORD  ed_OwnerGID;
};
struct ExAllControl {
	ULONG eac_Entries;
	ULONG eac_LastKey;
	UBYTE *eac_MatchString;
	struct Hook *eac_MatchFunc;
};

/* dos/dosextens.h */
#define ST_FILE             -3
#define ST_ROOT              1
#define ST_USERDIR           2
#define ST_LINKDIR           4

/* dos/stdio.h */
#define ENDSTREAMCH         -1

/* dos/filehandler.h */
struct DosEnvec {
	IPTR de_TableSize;
	IPTR de_SizeBlock;
	IPTR de_SecOrg;
	IPTR de_Surfaces;
	IPTR de_SectorPerBlock;
	IPTR de_BlocksPerTrack;
	IPTR de_Reserved;
	IPTR de_PerAlloc;
	IPTR de_Interleave;
	IPTR de_LowCyl;
	IPTR de_HighCyl;
	IPTR de_NumBuffers;
	IPTR de_BufMemType;
	IPTR de_MaxTransfer;
	IPTR de_Mask;
	IPTR de_BootPri;
	IPTR de_DosType;
	IPTR de_Baud;
	IPTR de_Control;
	IPTR de_BootBlocks;
};

/* dos/filesystem.h */
#define FMF_LOCK   (1L<<0)
#define FMF_WRITE  (1L<<2)
#define FMF_READ   (1L<<3)
#define FMF_CREATE (1L<<4)
#define FMF_CLEAR  (1L<<5)

/* aros/debug.h */
#if DEBUG
#	define D(x) x
#else
#	define D(x)
#endif
#define bug kprintf
#define kprintf printf

/* (aros/macros.h) */
#define OS_BE2LONG ntohl
#define OS_LONG2BE htonl

/* (aros/machine.h) */
#define OS_PTRALIGN (sizeof(APTR))

/* proto/exec.h */
APTR AllocMem(ULONG, ULONG);
APTR AllocVec(ULONG, ULONG);
void CopyMem(APTR, APTR, ULONG);
void FreeMem(APTR, ULONG);
void FreeVec(APTR);
/* exec/memory.h */
#define MEMF_PUBLIC (1L<<0)
#define MEMF_CLEAR  (1L<<16)

/* proto/dos.h */
struct DateStamp *DateStamp(struct DateStamp *);
STRPTR PathPart(STRPTR);

/* io */
struct IOHandle {
	STRPTR blockdevice;
	ULONG unit;
	ULONG flags;
	FILE *fh;
	void *mediachangedata;
	ULONG ioflags;
};

#define IOHF_DISK_IN      (1<<2)

#endif

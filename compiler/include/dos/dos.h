#ifndef DOS_DOS_H
#define DOS_DOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Basic structures and constants
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_MACHINE_H
#   include <aros/machine.h>
#endif

/*
    Replace BPTRs by simple APTRs for some machines. On Amiga with binary
    compatibility, this would look like this:

    typedef ULONG BPTR;
    #define MKBADDR(a)      (((BPTR)(a))>>2)
    #define BADDR(a)        (((APTR)(a))<<2)
*/
#ifndef AROS_BPTR_TYPE
#   define AROS_FAST_BPTR
#   define AROS_BPTR_TYPE   APTR
#   define MKBADDR(a)       ((APTR)(a))
#   define BADDR(a)         (a)
#endif
#ifndef AROS_BSTR_TYPE
#   define AROS_BSTR_TYPE   STRPTR
#endif

#ifndef __typedef_BPTR
#   define __typedef_BPTR
    typedef AROS_BPTR_TYPE BPTR;
#endif
#ifndef __typedef_BSTR
#   define __typedef_BSTR
    typedef AROS_BSTR_TYPE BSTR;
#endif

#define DOSNAME "dos.library"

struct DateStamp
{
   LONG ds_Days;
   LONG ds_Minute;
   LONG ds_Tick;
};

struct FileInfoBlock
{
    LONG	     fib_DiskKey;
    LONG	     fib_DirEntryType;
    UBYTE	     fib_FileName[108];
    LONG	     fib_Protection;
    LONG	     fib_EntryType;
    LONG	     fib_Size;
    LONG	     fib_NumBlocks;
    struct DateStamp fib_Date;
    UBYTE	     fib_Comment[80];
    UWORD	     fib_OwnerUID;
    UWORD	     fib_OwnerGID;
    UBYTE	     fib_Reserved[32];
};

#define DOSTRUE 		(-1L)
#define DOSFALSE		( 0L)

#define MODE_OLDFILE		1005
#define MODE_NEWFILE		1006
#define MODE_READWRITE		1004

#define SHARED_LOCK		-2
#define ACCESS_READ		SHARED_LOCK
#define EXCLUSIVE_LOCK		-1
#define ACCESS_WRITE		EXCLUSIVE_LOCK

#ifndef BITSPERBYTE
#   define BITSPERBYTE		8
#endif
#ifndef BYTESPERLONG
#   define BYTESPERLONG 	4
#endif
#define BITSPERLONG		32 /* This is (U)LONG, not long ! */
#ifndef MAXINT
#   define MAXINT		0x7FFFFFFF
#endif
#ifndef MININT
#   define MININT		0x80000000
#endif

#define OFFSET_BEGINNING       -1
#define OFFSET_CURRENT		0
#define OFFSET_END		1

#define RETURN_OK		0
#define RETURN_WARN		5
#define RETURN_ERROR		10
#define RETURN_FAIL		20

#define SIGBREAKB_CTRL_C	12
#define SIGBREAKB_CTRL_D	13
#define SIGBREAKB_CTRL_E	14
#define SIGBREAKB_CTRL_F	15
#define SIGBREAKF_CTRL_C	0x1000L
#define SIGBREAKF_CTRL_D	0x2000L
#define SIGBREAKF_CTRL_E	0x4000L
#define SIGBREAKF_CTRL_F	0x8000L

#define ITEM_EQUAL	       -2
#define ITEM_ERROR	       -1
#define ITEM_NOTHING		0
#define ITEM_UNQUOTED		1
#define ITEM_QUOTED		2

#define DOS_FILEHANDLE		0
#define DOS_EXALLCONTROL	1
#define DOS_FIB 		2
#define DOS_STDPKT		3
#define DOS_CLI 		4
#define DOS_RDARGS		5

#define TICKS_PER_SECOND	50

#define FIBB_DELETE		0
#define FIBB_EXECUTE		1
#define FIBB_WRITE		2
#define FIBB_READ		3
#define FIBB_ARCHIVE		4
#define FIBB_PURE		5
#define FIBB_SCRIPT		6
#define FIBB_GRP_DELETE 	8
#define FIBB_GRP_EXECUTE	9
#define FIBB_GRP_WRITE		10
#define FIBB_GRP_READ		11
#define FIBB_OTR_DELETE 	12
#define FIBB_OTR_EXECUTE	13
#define FIBB_OTR_WRITE		14
#define FIBB_OTR_READ		15
#define FIBF_DELETE		0x0001
#define FIBF_EXECUTE		0x0002
#define FIBF_WRITE		0x0004
#define FIBF_READ		0x0008
#define FIBF_ARCHIVE		0x0010
#define FIBF_PURE		0x0020
#define FIBF_SCRIPT		0x0040
#define FIBF_GRP_DELETE 	0x0100
#define FIBF_GRP_EXECUTE	0x0200
#define FIBF_GRP_WRITE		0x0400
#define FIBF_GRP_READ		0x0800
#define FIBF_OTR_DELETE 	0x1000
#define FIBF_OTR_EXECUTE	0x2000
#define FIBF_OTR_WRITE		0x4000
#define FIBF_OTR_READ		0x8000

#define ERROR_NO_FREE_STORE		103
#define ERROR_TASK_TABLE_FULL		105
#define ERROR_BAD_TEMPLATE		114
#define ERROR_BAD_NUMBER		115
#define ERROR_REQUIRED_ARG_MISSING	116
#define ERROR_KEY_NEEDS_ARG		117
#define ERROR_TOO_MANY_ARGS		118
#define ERROR_UNMATCHED_QUOTES		119
#define ERROR_LINE_TOO_LONG		120
#define ERROR_FILE_NOT_OBJECT		121
#define ERROR_INVALID_RESIDENT_LIBRARY	122
#define ERROR_NO_DEFAULT_DIR		201
#define ERROR_OBJECT_IN_USE		202
#define ERROR_OBJECT_EXISTS		203
#define ERROR_DIR_NOT_FOUND		204
#define ERROR_OBJECT_NOT_FOUND		205
#define ERROR_BAD_STREAM_NAME		206
#define ERROR_OBJECT_TOO_LARGE		207
#define ERROR_ACTION_NOT_KNOWN		209
#define ERROR_INVALID_COMPONENT_NAME	210
#define ERROR_INVALID_LOCK		211
#define ERROR_OBJECT_WRONG_TYPE 	212
#define ERROR_DISK_NOT_VALIDATED	213
#define ERROR_DISK_WRITE_PROTECTED	214
#define ERROR_RENAME_ACROSS_DEVICES	215
#define ERROR_DIRECTORY_NOT_EMPTY	216
#define ERROR_TOO_MANY_LEVELS		217
#define ERROR_DEVICE_NOT_MOUNTED	218
#define ERROR_SEEK_ERROR		219
#define ERROR_COMMENT_TOO_BIG		220
#define ERROR_DISK_FULL 		221
#define ERROR_DELETE_PROTECTED		222
#define ERROR_WRITE_PROTECTED		223
#define ERROR_READ_PROTECTED		224
#define ERROR_NOT_A_DOS_DISK		225
#define ERROR_NO_DISK			226
#define ERROR_NO_MORE_ENTRIES		232
#define ERROR_IS_SOFT_LINK		233
#define ERROR_OBJECT_LINKED		234
#define ERROR_BAD_HUNK			235
#define ERROR_NOT_IMPLEMENTED		236
#define ERROR_RECORD_NOT_LOCKED 	240
#define ERROR_LOCK_COLLISION		241
#define ERROR_LOCK_TIMEOUT		242
#define ERROR_UNLOCK_ERROR		243

#define FAULT_MAX		82

struct InfoData
{
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

#define ID_WRITE_PROTECTED	80
#define ID_VALIDATING		81
#define ID_VALIDATED		82

/* Filesystem types (multi-character constants of identifier strings) */
#define ID_NO_DISK_PRESENT	(-1L)
#define ID_UNREADABLE_DISK	(0x42414400L)   /* 'BAD\0' */
#define ID_DOS_DISK		(0x444F5300L)   /* 'DOS\0' */
#define ID_FFS_DISK		(0x444F5301L)   /* 'DOS\1' */
#define ID_INTER_DOS_DISK	(0x444F5302L)   /* 'DOS\2' */
#define ID_INTER_FFS_DISK	(0x444F5303L)   /* 'DOS\3' */
#define ID_FASTDIR_DOS_DISK	(0x444F5304L)   /* 'DOS\4' */
#define ID_FASTDIR_FFS_DISK	(0x444F5305L)   /* 'DOS\5' */
#define ID_NOT_REALLY_DOS	(0x4E444F53L)   /* 'NDOS'  */
#define ID_KICKSTART_DISK	(0x4B49434BL)   /* 'KICK'  */
#define ID_MSDOS_DISK		(0x4d534400L)   /* 'MSD\0' */

#define LOCK_DIFFERENT	       -1
#define LOCK_SAME		0
#define LOCK_SAME_VOLUME	1

#define CHANGE_LOCK		0
#define CHANGE_FH		1

#define LINK_HARD		0
#define LINK_SOFT		1

#endif /* DOS_DOS_H */

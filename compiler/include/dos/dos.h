#ifndef DOS_DOS_H
#define DOS_DOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Basic DOS structures and constants
    Lang: english
*/
#ifndef _VALUES_H
#   include <values.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

/* The name of the dos.library. Use this constant instead of a string. */
#define DOSNAME "dos.library"

#define DOSTRUE  (-1L)
#define DOSFALSE ( 0L)

/**********************************************************************
 ******************************* Dates ********************************
 **********************************************************************/

#define TICKS_PER_SECOND 50

/* DateStamp structure as used in different library-functions. This
   structure sufficiently describes a system-date and -time (i.e. any
   date since 1.1.1978). */
struct DateStamp
{
   LONG ds_Days;   /* Number of days since 1.1.1978 */
   LONG ds_Minute; /* Number of minutes since midnight */
   LONG ds_Tick;   /* Number of ticks (1/50 second) in the current minute
                      Note that this may not be exact */
};

/**********************************************************************
 ******************************* Files ********************************
 **********************************************************************/

/* The maximum length of filenames in AmigaOS. You should not depend on
   this value, as it may change in future versions. */
#define MAXFILENAMELENGTH 108

/* Structure used to describe a directory entry. Note that not all fields
   are supported by all filesystems. This structure should be allocated
   with AllocDosObject(). */
struct FileInfoBlock
{
    LONG	     fib_DiskKey;
    LONG	     fib_DirEntryType; /* Type of entry. If the uppermost
                                          bit is set, this is a file, if
                                          not, it is a directory */
    UBYTE	     fib_FileName[MAXFILENAMELENGTH];
                                       /* The name of the entry
                                          (null-terminated) */
    LONG	     fib_Protection;   /* The protection bits (see below) */
    LONG	     fib_EntryType;
    LONG	     fib_Size;         /* The size of the file */
    LONG	     fib_NumBlocks;    /* Number of blocks used for file */
    struct DateStamp fib_Date;         /* Date of last change to file */
    UBYTE	     fib_Comment[80];  /* The filecomment (null-terminated) */
    UWORD	     fib_OwnerUID;     /* UsedID of fileowner */
    UWORD	     fib_OwnerGID;     /* GroupID of fileowner */
    UBYTE	     fib_Reserved[32]; /* PRIVATE */
};

/* Protection bits for files */
/* Flags for owner (they a low-active, i.e. not set means the action is
   allowed!) */
#define FIBB_DELETE      0  /* File is deleteable */
#define FIBB_EXECUTE     1  /* File is executable (programs only) */
#define FIBB_WRITE       2  /* File is writable */
#define FIBB_READ        3  /* File is readable */
/* General flags, not owner-dependant */
#define FIBB_ARCHIVE     4  /* File was archived (not used by OS) */
#define FIBB_PURE        5
#define FIBB_SCRIPT      6  /* File is a script (DOS or ARexx) */
/* Flags for group (meaning see above, high-active!) */
#define FIBB_GRP_DELETE  8
#define FIBB_GRP_EXECUTE 9
#define FIBB_GRP_WRITE   10
#define FIBB_GRP_READ    11
/* Flags for other/world (meaning see above, high-active!) */
#define FIBB_OTR_DELETE  12
#define FIBB_OTR_EXECUTE 13
#define FIBB_OTR_WRITE   14
#define FIBB_OTR_READ    15

#define FIBF_DELETE      (1<<FIBB_DELETE)
#define FIBF_EXECUTE     (1<<FIBB_EXECUTE)
#define FIBF_WRITE       (1<<FIBB_WRITE)
#define FIBF_READ        (1<<FIBB_READ)
#define FIBF_ARCHIVE     (1<<FIBB_ARCHIVE)
#define FIBF_PURE        (1<<FIBB_PURE)
#define FIBF_SCRIPT      (1<<FIBB_SCRIPT)
#define FIBF_GRP_DELETE  (1<<FIBB_GRP_DELETE)
#define FIBF_GRP_EXECUTE (1<<FIBB_GRP_EXECUTE)
#define FIBF_GRP_WRITE   (1<<FIBB_GRP_WRITE)
#define FIBF_GRP_READ    (1<<FIBB_GRP_READ)
#define FIBF_OTR_DELETE  (1<<FIBB_OTR_DELETE)
#define FIBF_OTR_EXECUTE (1<<FIBB_OTR_EXECUTE)
#define FIBF_OTR_WRITE   (1<<FIBB_OTR_WRITE)
#define FIBF_OTR_READ    (1<<FIBB_OTR_READ)

/**********************************************************************
 ****************************** Devices *******************************
 **********************************************************************/

/* Structure used in Info(). Must be longword-aligned. */
struct InfoData
{
    LONG id_NumSoftErrors; /* Number of soft errors on device */
    LONG id_UnitNumber;    /* Unit number of device */
    LONG id_DiskState;     /* see below */
    LONG id_NumBlocks;     /* Number of blocks on device */
    LONG id_NumBlocksUsed; /* Number of blocks in use */
    LONG id_BytesPerBlock; /* Bytes per block */
    LONG id_DiskType;      /* Type of disk (see below) */
    BPTR id_VolumeNode;
    LONG id_InUse;         /* Set if device is in use */
};

/* id_DiskState */
#define ID_WRITE_PROTECTED 80 /* Device is write-protected */
#define ID_VALIDATING      81 /* Device is validating */
#define ID_VALIDATED       82 /* Device is read-write */

/* id_DiskType, filesystem types (multi-character constants of identifier
   strings)
*/
#define ID_NO_DISK_PRESENT  (-1L)
#define ID_UNREADABLE_DISK  MAKE_ID('B','A','D',0)
#define ID_DOS_DISK         MAKE_ID('D','O','S',0)
#define ID_FFS_DISK         MAKE_ID('D','O','S',1)
#define ID_INTER_DOS_DISK   MAKE_ID('D','O','S',2)
#define ID_INTER_FFS_DISK   MAKE_ID('D','O','S',3)
#define ID_FASTDIR_DOS_DISK MAKE_ID('D','O','S',4)
#define ID_FASTDIR_FFS_DISK MAKE_ID('D','O','S',5)
#define ID_NOT_REALLY_DOS   MAKE_ID('N','D','O','S')
#define ID_KICKSTART_DISK   MAKE_ID('K','I','C','K')
#define ID_MSDOS_DISK       MAKE_ID('M','S','D',0)

/**********************************************************************
 **************** Program Execution and Error Handling ****************
 **********************************************************************/

/* Return values for programs */
#define RETURN_OK    0  /* Program succeeded */
#define RETURN_WARN  5  /* Program succeeded, but there was something not
                           quite right. This value may also be used to
                           express a boolean state (RETURN_WARN meaning
                           TRUE, RETURN_OK meaning FALSE). */
#define RETURN_ERROR 10 /* Program succeeded partly. This may be returned,
                           if the user aborts a program or some external
                           data were wrong. */
#define RETURN_FAIL  20 /* Program execution failed, because it couldn't
                           allocate system resources. */

/* Secondary errors values as used for IoErr(), SetIoErr() and in
   Process->pr_Result2.
*/
/* General system errors */
#define ERROR_NO_FREE_STORE		103 /* Out of memory */
#define ERROR_TASK_TABLE_FULL		105 /* Too many tasks running */
/* Errors concerning ReadArgs() */
#define ERROR_BAD_TEMPLATE		114 /* Supplied template is broken */
#define ERROR_BAD_NUMBER		115 /* Numeric arg is not numeric */
#define ERROR_REQUIRED_ARG_MISSING	116
#define ERROR_KEY_NEEDS_ARG		117
#define ERROR_TOO_MANY_ARGS		118
#define ERROR_UNMATCHED_QUOTES		119 /* Odd number of quotation marks */
#define ERROR_LINE_TOO_LONG		120 /* Hardcoded line length limit
                                               passed */
/* File errors */
#define ERROR_FILE_NOT_OBJECT		121
#define ERROR_INVALID_RESIDENT_LIBRARY	122
#define ERROR_NO_DEFAULT_DIR		201
#define ERROR_OBJECT_IN_USE		202 /* Object already in use */
#define ERROR_OBJECT_EXISTS		203 /* Object does already exist */
#define ERROR_DIR_NOT_FOUND		204
#define ERROR_OBJECT_NOT_FOUND		205
/* Miscellaneous errors */
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

/* Maximum length of strings got from Fault(). Note that they should be
   shorter than 60 characters.
*/
#define FAULT_MAX		82

/* Signals that are set, if the user presses the corresponding keys on
   the controlling terminal. They may also be sent by using Singal().
   For more information see <exec/tasks.h>.
*/
#define SIGBREAKB_CTRL_C 12 /* CTRL-c, usually meaning program abortion */
#define SIGBREAKB_CTRL_D 13 /* CTRL-d */
#define SIGBREAKB_CTRL_E 14 /* CTRL-e */
#define SIGBREAKB_CTRL_F 15 /* CTRL-f */
#define SIGBREAKF_CTRL_C (1L<<SIGBREAKB_CTRL_C)
#define SIGBREAKF_CTRL_D (1L<<SIGBREAKB_CTRL_D)
#define SIGBREAKF_CTRL_E (1L<<SIGBREAKB_CTRL_E)
#define SIGBREAKF_CTRL_F (1L<<SIGBREAKB_CTRL_F)

/**********************************************************************
 ********************** Constants for Functions ***********************
 **********************************************************************/

/* Modes as used in Open() */
#define MODE_OLDFILE   1005 /* An old file is opened. If it doesn't exist,
                               Open() returns an error. */
#define MODE_NEWFILE   1006 /* A new file is created, even if a file with
                               the supplied name does already exist. */
#define MODE_READWRITE 1004 /* An old file is opened, if it doesn't exist,
                               a new one is created. */

/* Locking mechanism as used in Lock() */
#define SHARED_LOCK    -2
#define ACCESS_READ    SHARED_LOCK
#define EXCLUSIVE_LOCK -1
#define ACCESS_WRITE   EXCLUSIVE_LOCK

/* Returned by SameLock() */
#define LOCK_DIFFERENT   -1
#define LOCK_SAME         0
#define LOCK_SAME_VOLUME  1

/* Used in MakeLink() */
#define LINK_HARD 0
#define LINK_SOFT 1

/* Used in Seek() */
#define OFFSET_BEGINNING -1
#define OFFSET_CURRENT    0
#define OFFSET_END        1

/* Used in ChangeMode() */
#define CHANGE_LOCK 0
#define CHANGE_FH   1

/* Returned by ReadItem() */
#define ITEM_EQUAL    -2
#define ITEM_ERROR    -1
#define ITEM_NOTHING   0
#define ITEM_UNQUOTED  1
#define ITEM_QUOTED    2

/* The types for AllocDosObject() and FreeDosObject(). They specifiy which
   kind of structure is to be allocated.
*/
#define DOS_FILEHANDLE   0 /* struct FileHandle <dos/dosextens.h> */
#define DOS_EXALLCONTROL 1 /* struct ExAllControl <dos/exall.h> */
#define DOS_FIB          2 /* struct FileInfoBlock (see above) */
#define DOS_STDPKT       3 /* struct DosPacket <dos/dosextens.h> */
#define DOS_CLI          4 /* struct CommandLineInterface <dos/dosextens.h> */
#define DOS_RDARGS       5 /* struct RDArgs <dos/rdargs.h> */

#endif /* DOS_DOS_H */

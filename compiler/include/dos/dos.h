#ifndef DOS_DOS_H
#define DOS_DOS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Basic DOS structures and constants
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef AROS_MACROS_H
#   include <aros/macros.h>
#endif

/* The name of the dos.library. Use this constant instead of the string. */
#define DOSNAME "dos.library"

/* These constants may be used, but preferably, you should only test for
   (un-)equality against DOSFALSE (i.e. 0). */
#define DOSTRUE  (-1L)
#define DOSFALSE ( 0L)

/**********************************************************************
 ******************************* Dates ********************************
 **********************************************************************/

/* Other structures and defines for handling dates and time can be found in
   <dos/datetime.h>. */

/* Number of so-called "ticks" in a second. */
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

/* The maximum length of comments in AmigaOS. You should not depend on
   this value, as it may change in future versions. */
#define MAXCOMMENTLENGTH 80

/* Structure used to describe a directory entry. Note that not all fields
   are supported by all filesystems. This structure should be allocated
   with AllocDosObject(). */
struct FileInfoBlock
{
    LONG	     fib_DiskKey;
      /* See <dos/dosextens.h> for definitions. Generally: if this is >= 0
         the file described is a directory, otherwise it is a plain file. */
    LONG	     fib_DirEntryType;
      /* The filename (null-terminated). */
    UBYTE	     fib_FileName[MAXFILENAMELENGTH];
    LONG	     fib_Protection;   /* The protection bits (see below). */
    LONG	     fib_EntryType;
    LONG	     fib_Size;         /* The size of the file. */
    LONG	     fib_NumBlocks;    /* Number of blocks used for file. */
    struct DateStamp fib_Date;         /* Date of last change to file. */
    UBYTE	     fib_Comment[MAXCOMMENTLENGTH];  /* The filecomment (null-terminated). */
    UWORD	     fib_OwnerUID;     /* UserID of fileowner. */
    UWORD	     fib_OwnerGID;     /* GroupID of fileowner. */
    UBYTE	     fib_Reserved[32]; /* PRIVATE */
};

/* Protection bits for files (fib_Protection). */
/* Flags for owner (they a low-active, i.e. not set means the action is
   allowed!) */
#define FIBB_DELETE      0  /* File is deleteable. */
#define FIBB_EXECUTE     1  /* File is executable (no scripts!). */
#define FIBB_WRITE       2  /* File is writable. */
#define FIBB_READ        3  /* File is readable. */
/* General flags, not owner-dependant. */
#define FIBB_ARCHIVE     4  /* File was archived (not used by OS). */
#define FIBB_PURE        5  /* Make program resident on execution. */
#define FIBB_SCRIPT      6  /* File is a script (DOS or ARexx). */
/* Flag number 7 is not defined. It used to describe different conditions
   in different revisions of AmigaOS and was also misused as hidden flag.
   Because of this confusion, this flag should not be used! */
/* Flags for group (meaning see above, these are high-active!). */
#define FIBB_GRP_DELETE  8
#define FIBB_GRP_EXECUTE 9
#define FIBB_GRP_WRITE   10
#define FIBB_GRP_READ    11
/* Flags for other/world (meaning see above, these are high-active!). */
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
    LONG id_NumSoftErrors; /* Number of soft errors on device. */
    LONG id_UnitNumber;    /* Unit number of device. */
    LONG id_DiskState;     /* State the current volume is in (see below). */
    LONG id_NumBlocks;     /* Number of blocks on device. */
    LONG id_NumBlocksUsed; /* Number of blocks in use. */
    LONG id_BytesPerBlock; /* Bytes per block. */
    LONG id_DiskType;      /* Type of disk (see below). */
    BPTR id_VolumeNode;
    LONG id_InUse;         /* Set, if device is in use. */
};

/* id_DiskState */
#define ID_WRITE_PROTECTED 80 /* Volume is write-protected. */
#define ID_VALIDATING      81 /* Volume is currently validating. */
#define ID_VALIDATED       82 /* Volume is ready to be read and written. */

/* Filesystem types as used for id_DiskType. These are multi-character
   constants of identifier strings. They are self-descriptive. */
#define ID_NO_DISK_PRESENT  (-1L)
#define ID_UNREADABLE_DISK  AROS_MAKE_ID('B','A','D',0)
#define ID_DOS_DISK         AROS_MAKE_ID('D','O','S',0)
#define ID_FFS_DISK         AROS_MAKE_ID('D','O','S',1)
#define ID_INTER_DOS_DISK   AROS_MAKE_ID('D','O','S',2)
#define ID_INTER_FFS_DISK   AROS_MAKE_ID('D','O','S',3)
#define ID_FASTDIR_DOS_DISK AROS_MAKE_ID('D','O','S',4)
#define ID_FASTDIR_FFS_DISK AROS_MAKE_ID('D','O','S',5)
#define ID_NOT_REALLY_DOS   AROS_MAKE_ID('N','D','O','S')
#define ID_KICKSTART_DISK   AROS_MAKE_ID('K','I','C','K')
#define ID_MSDOS_DISK       AROS_MAKE_ID('M','S','D',0)

/**********************************************************************
 **************** Program Execution and Error Handling ****************
 **********************************************************************/

/* Return conditions for programs. */
#define RETURN_OK    0
  /* Program succeeded. */
#define RETURN_WARN  5
  /* Program succeeded, but there was something not quite right. This value
     may also be used to express a boolean state (RETURN_WARN meaning TRUE,
     RETURN_OK meaning FALSE). */
#define RETURN_ERROR 10
  /* Program succeeded partly. This may be returned, if the user aborts a
     program or some external data were wrong. */
#define RETURN_FAIL  20
  /* Program execution failed. Normally used, if some system resources could
     not be allocated. */


/* Secondary errors codes as used for IoErr(), SetIoErr() and in
   Process->pr_Result2. The term 'object' refers to files of all kinds
   (ie plain files, directories, links, etc). */

  /* This is used, if something went wrong, but it is unknown what exactly
     went wrong. This is especially useful for emulation devices, when the
     underlying system returned an error that the emulation side does not
     know. */
#define ERROR_UNKNOWN			100


/* General system errors */
  /* Out of memory. */
#define ERROR_NO_FREE_STORE		103
  /* Too many tasks are already running. */
#define ERROR_TASK_TABLE_FULL		105

/* Errors concerning ReadArgs(). See also <dos/rdargs.h>. */
  /* Supplied template is broken */
#define ERROR_BAD_TEMPLATE		114
  /* A supplied argument that was expected to be numeric, was not numeric.
     This is also returned by some functions to expresss that a supplied
     number is out of range (ie to express application internal errors). */
#define ERROR_BAD_NUMBER		115
  /* An argument that has to be supplied (ie signed with the '/A' flag) was
     not supplied. */
#define ERROR_REQUIRED_ARG_MISSING	116
  /* Keyword was specified, but not its contents. */
#define ERROR_KEY_NEEDS_ARG		117
  /* There were more arguments than the template needs. */
#define ERROR_TOO_MANY_ARGS		118
  /* An odd number of quotation marks was supplied. */
#define ERROR_UNMATCHED_QUOTES		119
  /* Either the command-line was longer than hardcoded line length limit or the
     maximum number of multiple arguments (flag '/M') was exceeded. This can
     also indicate that some argument is too long or a supplied buffer is too
     small. */
#define ERROR_LINE_TOO_LONG		120

/* Errors in files. */
  /* You tried to execute a file that is not an executable. */
#define ERROR_FILE_NOT_OBJECT		121
  /* A library or device could not be opened or that library or device is
     broken. */
#define ERROR_INVALID_RESIDENT_LIBRARY	122
#define ERROR_NO_DEFAULT_DIR		201
  /* The accessed object is already in use (eg locked) by another task. */
#define ERROR_OBJECT_IN_USE		202
  /* You tried to overwrite an object. */
#define ERROR_OBJECT_EXISTS		203
  /* The given directory or the path of a given object does not exist. */
#define ERROR_DIR_NOT_FOUND		204
  /* The given object does not exist. */
#define ERROR_OBJECT_NOT_FOUND		205

/* Miscellaneous errors. */
#define ERROR_BAD_STREAM_NAME		206
  /* The given object is too large for the operation to be made. Object is
     this context are for example components of path-names. */
#define ERROR_OBJECT_TOO_LARGE		207
  /* This is usually used to indicate that a filesystem does not support a
     certain action, but may generally also be used by functions. */
#define ERROR_ACTION_NOT_KNOWN		209
  /* A path component was invalid (eg there were multiple colons in a path
     name). */
#define ERROR_INVALID_COMPONENT_NAME	210
#define ERROR_INVALID_LOCK		211
  /* You tried to perform an action on an object, which this kind of object
     does not support (eg makedir on a file). */
#define ERROR_OBJECT_WRONG_TYPE 	212
  /* Writing failed, because the volume is not validated. */
#define ERROR_DISK_NOT_VALIDATED	213
  /* Writing failed, because the volume is write-protected. */
#define ERROR_DISK_WRITE_PROTECTED	214
  /* You tried to move/rename a file across different devices. Rename does only
     work on the same device, as only the inode-data has to be changed to
     perform that action. */
#define ERROR_RENAME_ACROSS_DEVICES	215
  /* You tried to delete a directory that still contains some files. Delete
     these files first. */
#define ERROR_DIRECTORY_NOT_EMPTY	216
  /* A recursive directory search could not be performed, because the stack
     was too small. */
#define ERROR_TOO_MANY_LEVELS		217
  /* You tried to access a device that is currently not mounted. */
#define ERROR_DEVICE_NOT_MOUNTED	218
  /* An error occured, while executing Seek(). */
#define ERROR_SEEK_ERROR		219
  /* The supplied file comment was longer than the hardcoded length limit for
     file comments. */
#define ERROR_COMMENT_TOO_BIG		220
  /* A write-operation could not be performed, because the volume has no space
     left. */
#define ERROR_DISK_FULL 		221
  /* You tried to delete a delete-protected object. */
#define ERROR_DELETE_PROTECTED		222
  /* You tried to write to a write-protected object. This does not mean that
     the volume, you wanted to write to, is write-protected! */
#define ERROR_WRITE_PROTECTED		223
  /* You tried to read a read-protected object. */
#define ERROR_READ_PROTECTED		224
  /* Accessed disk is unreadable. */
#define ERROR_NOT_A_DOS_DISK		225
  /* You tried to perform an action on a device, which has no volume mounted
     (eg an empty disk drive). */
#define ERROR_NO_DISK			226
  /* This does not indicate an error, but is returned by several functions to
     indicate that the last entry of a list was reached. */
#define ERROR_NO_MORE_ENTRIES		232
  /* Given action can not be performed on a given object, because it is a
     soft-link. This is usually only used by filesystem handlers and is catched
     by dos. Applications should not see this. */
#define ERROR_IS_SOFT_LINK		233
  /* Given action can not be performed on a given object, because it is a link.
  */
#define ERROR_OBJECT_LINKED		234
  /* There was a bad hunk in a file that was to load. */
#define ERROR_BAD_HUNK			235
  /* Indicates that a function does not implement a certain functionality.
     There are more special error conditions (ERROR_BAD_NUMBER and
     ERROR_ACTION_NOT_KNOWN), which should be preferred, if applicable. */
#define ERROR_NOT_IMPLEMENTED		236
  /* You tried to access a record that was not locked. */
#define ERROR_RECORD_NOT_LOCKED 	240
  /* Somebody already locked a part of the record, you wanted to lock. */
#define ERROR_LOCK_COLLISION		241
  /* LockRecord() timed out. */
#define ERROR_LOCK_TIMEOUT		242
  /* An error occured, while unlocking a record. */
#define ERROR_UNLOCK_ERROR		243

/* Further errors are defined in dosasl.h amd filesystem.h as well */

/* Maximum length of strings got from Fault(). Note that they should be
   shorter than 60 characters. */
#define FAULT_MAX		82

/* Signals that are set, if the user presses the corresponding keys on
   the controlling terminal. They may also be sent by using Signal().
   For more information see <exec/tasks.h>. */
#define SIGBREAKB_CTRL_C 12 /* CTRL-c, usually meaning program abortion. */
#define SIGBREAKB_CTRL_D 13 /* CTRL-d */
#define SIGBREAKB_CTRL_E 14 /* CTRL-e, usually meaning that the application
                               should iconify itself. */
#define SIGBREAKB_CTRL_F 15 /* CTRL-f, usually meaning that the application
                               should uniconify itself. */

#define SIGBREAKF_CTRL_C (1L<<SIGBREAKB_CTRL_C)
#define SIGBREAKF_CTRL_D (1L<<SIGBREAKB_CTRL_D)
#define SIGBREAKF_CTRL_E (1L<<SIGBREAKB_CTRL_E)
#define SIGBREAKF_CTRL_F (1L<<SIGBREAKB_CTRL_F)

/**********************************************************************
 ********************** Constants for Functions ***********************
 **********************************************************************/

/* Modes for Open(). */
  /* Try to open old file. If it does not exist, Open() returns an error. */
#define MODE_OLDFILE   1005
  /* A new file is created, even if a file with the supplied name does
     already exist. */
#define MODE_NEWFILE   1006
  /* An old file is opened. If it does not exist, a new one is created. */
#define MODE_READWRITE 1004

/* Locking mechanism as used in Lock() */
/* Non-exclusive lock, other tasks may lock this file as well. This is used
   for read-only operations. */
#define SHARED_LOCK    -2
#define ACCESS_READ    SHARED_LOCK
/* Exclusive lock, other tasks may not lock this file. This is used for write
   operations. */
#define EXCLUSIVE_LOCK -1
#define ACCESS_WRITE   EXCLUSIVE_LOCK

/* Returned by SameLock(). See autodocs for description. */
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

/* Limits of the "int" type */
#define MAXINT ((int)(((unsigned int)~0)/2))
#define MININT (-MAXINT - 1)

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
   kind of structure is to be allocated. */
#define DOS_FILEHANDLE   0 /* struct FileHandle <dos/dosextens.h> */
#define DOS_EXALLCONTROL 1 /* struct ExAllControl <dos/exall.h> */
#define DOS_FIB          2 /* struct FileInfoBlock (see above) */
#define DOS_STDPKT       3 /* struct DosPacket <dos/dosextens.h> */
#define DOS_CLI          4 /* struct CommandLineInterface <dos/dosextens.h> */
#define DOS_RDARGS       5 /* struct RDArgs <dos/rdargs.h> */

#endif /* DOS_DOS_H */

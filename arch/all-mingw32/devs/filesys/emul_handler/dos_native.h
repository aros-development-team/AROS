/* This file contains some definitions ripped from dos.library includes. I can't use them *
 * directly because some defines in dos.h conflict with defines in Windows headers.       */

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
    void *id_VolumeNode;
    LONG id_InUse;         /* Set, if device is in use. */
};

/* id_DiskState */
#define ID_WRITE_PROTECTED 80 /* Volume is write-protected. */
#define ID_VALIDATING      81 /* Volume is currently validating. */
#define ID_VALIDATED       82 /* Volume is ready to be read and written. */

/* Filesystem types as used for id_DiskType. These are multi-character
   constants of identifier strings. They are self-descriptive. */
#define ID_NO_DISK_PRESENT  (-1L)
#define ID_DOS_DISK         0x444F5300

/* Constants, defining of what kind a file is. These constants are used in
   many structures, including FileInfoBlock (<dos/dos.h>) and ExAllData
   (<dos/exall.h>). */
#define ST_PIPEFILE -5 /* File is a pipe */
#define ST_LINKFILE -4 /* Hard link to a file */
#define ST_FILE     -3 /* Plain file */
#define ST_ROOT      1 /* Root directory of filesystem */
#define ST_USERDIR   2 /* Normal directory */
#define ST_SOFTLINK  3 /* Soft link (may be a file or directory) */
#define ST_LINKDIR   4 /* Hard link to a directory */

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
#define AROS_ERROR_OBJECT_NOT_FOUND	205

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
#define AROS_ERROR_DISK_FULL 		221
  /* You tried to delete a delete-protected object. */
#define ERROR_DELETE_PROTECTED		222
  /* You tried to write to a write-protected object. This does not mean that
     the volume, you wanted to write to, is write-protected! */
#define ERROR_WRITE_PROTECTED		223
  /* You tried to read a read-protected object. */
#define ERROR_READ_PROTECTED		224
  /* Accessed disk is unreadable. */
#define ERROR_NOT_A_DOS_DISK		225
  /* You tried to perform an action on a device that has no volume mounted
     (eg. an empty disk drive). */
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

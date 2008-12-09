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

#ifndef DOS_FILESYSTEM_H
#define DOS_FILESYSTEM_H
#include <exec/io.h>

struct IOFileSys
{
    struct IORequest IOFS; /* Standard I/O request */
    LONG io_DosError;	/* Dos error code */
    IPTR io_Args[4];	/* Array of Arguments (Ints and Pointers) */
};

/*
    Filesystem actions. Look into one of the filesystem sources for more
    information on required parameters.
*/
#define FSA_MOUNT		1
#define FSA_DISMOUNT		2
#define FSA_OPEN		3
#define FSA_CLOSE		4
#define FSA_READ		5
#define FSA_WRITE		6
#define FSA_SEEK		7
#define FSA_SET_FILE_SIZE	8
#define FSA_WAIT_CHAR		9
#define FSA_FILE_MODE		10
#define FSA_IS_INTERACTIVE	11
#define FSA_SAME_LOCK		12
#define FSA_EXAMINE		13
#define FSA_EXAMINE_ALL 	14
#define FSA_EXAMINE_ALL_END	15
#define FSA_OPEN_FILE		16
#define FSA_CREATE_DIR		17
#define FSA_CREATE_HARDLINK	18
#define FSA_CREATE_SOFTLINK	19
#define FSA_RENAME		20
#define FSA_READ_SOFTLINK	21
#define FSA_DELETE_OBJECT	22
#define FSA_SET_COMMENT 	23
#define FSA_SET_PROTECT 	24
#define FSA_SET_OWNER		25
#define FSA_SET_DATE		26
#define FSA_IS_FILESYSTEM	27
#define FSA_MORE_CACHE		28
#define FSA_FORMAT		29
#define FSA_MOUNT_MODE		30
#if 0
#define FSA_SERIALIZE_DISK	30
#define FSA_FLUSH		31
#define FSA_INHIBIT		32
#define FSA_WRITE_PROTECT	33
#define FSA_DISK_CHANGE 	34
#define FSA_ADD_NOTIFY		35
#define FSA_REMOVE_NOTIFY	36
#define FSA_DISK_INFO		37
#define FSA_CHANGE_SIGNAL	38
#define FSA_LOCK_RECORD 	39
#define FSA_UNLOCK_RECORD	40
#endif

/* Modes for FSA_OPEN_FILE and FSA_FILE_MODE */
#define FMF_LOCK	1 /* Lock exclusively */
#define FMF_EXECUTE	2 /* open for executing */
#define FMF_WRITE	4 /* open for writing */
#define FMF_READ	8 /* open for reading */
#define FMF_CREATE	16 /* create file if it doesn't exist */
#define FMF_CLEAR	32 /* clear file on open */
#define FMF_RAW 	64 /* switch cooked to raw and vice versa */

/* Mount modes */
#define MMF_READ	1 /* Mounted for reading */
#define MMF_WRITE	2 /* Mounted for writing */
#define MMF_READ_CACHE	4 /* Read cache enabled */
#define MMF_WRITE_CACHE 8 /* Write cache enabled */
#define MMF_OFFLINE	16 /* Filesystem doesn't use the device currently */
#define MMF_LOCKED	32 /* Mount mode is password protected */

#endif

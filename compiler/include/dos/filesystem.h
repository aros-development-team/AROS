#ifndef DOS_FILESYSTEM_H
#define DOS_FILESYSTEM_H
#include <exec/io.h>

struct IOFileSys
{
	struct IORequest IOFS;
	LONG io_DosError;	/* Dos error code */
	LONG io_Args[4];	/* Array of Arguments */
};

#define FSA_STARTUP		1
#define FSA_DIE 		2
#define FSA_IS_FILESYSTEM	3
#define FSA_OPEN		4
#define FSA_CLOSE		5
#define FSA_READ		6
#define FSA_WRITE		7
#define FSA_SEEK		8
#define FSA_IS_INTERACTIVE	9
#define FSA_EXAMINE		10
#define FSA_EXAMINE_ALL 	11
#define FSA_EXAMINE_ALL_END	12
#define FSA_LOCATE_OBJECT	13
#define FSA_FREE_LOCK		14
#define FSA_DELETE_OBJECT
#define FSA_MAKE_LINK
#define FSA_CREATE_DIR
#define FSA_RENAME
#define FSA_SET_PROTECT
#define FSA_SET_OWNER
#define FSA_SET_DATE
#define FSA_READ_LINK
#define FSA_DISK_INFO
#define FSA_SERIALIZE_DISK
#define FSA_MORE_CACHE
#define FSA_WAIT_CHAR
#define FSA_INFO
#define FSA_FLUSH
#define FSA_SET_COMMENT
#define FSA_TIMER
#define FSA_INHIBIT
#define FSA_DISK_TYPE
#define FSA_DISK_CHANGE
#define FSA_SCREEN_MODE
#define FSA_READ_RETURN
#define FSA_WRITE_RETURN
#define FSA_SET_FILE_SIZE
#define FSA_WRITE_PROTECT
#define FSA_SAME_LOCK
#define FSA_CHANGE_SIGNAL
#define FSA_FORMAT
#define FSA_CHANGE_MODE
#define FSA_ADD_NOTIFY
#define FSA_REMOVE_NOTIFY

/* Modes for FSA_LOCATE_OBJECT */
#define LMF_LOCK	1 /* lock exclusively */
#define LMF_EXECUTE	2 /* check execute flag */
#define LMF_WRITE	4 /* check write flag */
#define LMF_READ	8 /* check read flag */
#define LMF_FILE	16 /* check if object is file */
#define LMF_CREATE	32 /* create file if doesn't exist */
#define LMF_CLEAR	64 /* clear file on open */

#define ET_COMMENTSIZE	-2
#define ET_NAMESIZE	-1
#define ET_TYPE 	1 /* Direntry type */
#define ET_NAME 	2 /* name of the file or directory */
#define ET_PROTECT	3 /* Protection bits (all bits 1==set) */
#define ET_UNITNUMBER	4 /* Which unit disk is mounted on */
#define ET_DISKSTATE	5 /* Inserted, removed, etc */
#define ET_SIZEH	6
#define ET_SIZE 	7 /* Filesize in bytes or volumesize in blocks */
#define ET_BLOCKSUSEDH	8
#define ET_BLOCKSUSED	9 /* Blocks needed for the file or used for the volume */
#define ET_DAYS 	10 /* Days since 1.1.78 */
#define ET_MINUTES	11 /* Minutes since midnight */
#define ET_TICKS	12 /* Ticks since full minute */
#define ET_COMMENT	13 /* comment */
#define ET_OWNERIDS	14 /* (UID<<16)+GID */
#define ET_BYTESPERBLK	15 /* Number of bytes per block */
#define ET_DISKTYPE	16 /* Filesystem ID */
#define ET_DOSLIST	17 /* Pointer to doslist entry */

#endif

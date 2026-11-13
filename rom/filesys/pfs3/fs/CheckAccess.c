/* $Id$ */
/* $Log: CheckAccess.c $
 * Revision 1.1  1996/01/03  10:18:38  Michiel
 * Initial revision
 * */

// own includes
#include "blocks.h"
#include "struct.h"
#include "disk_protos.h"
#include "allocation_protos.h"
#include "volume_protos.h"
#include "directory_protos.h"
#include "anodes_protos.h"
#include "update_protos.h"
#include "checkaccess_protos.h"

/* fileaccess that changes a file but doesn't write to it
 */
BOOL CheckChangeAccess(fileentry_t *file, SIPTR *error, globaldata *g)
{
#if DELDIR
	/* delfiles cannot be altered */
	if (IsDelFile (file->le.info))
	{
		*error = ERROR_WRITE_PROTECTED;
		return FALSE;
	}
#endif

	/* test on type */
	if (!IsFile(file->le.info)) 
	{
		*error = ERROR_OBJECT_WRONG_TYPE;
		return FALSE;
	}
	
	/* volume must be or become currentvolume */
	if (!CheckVolume(file->le.volume, 1, error, g))
		return FALSE;

	/* check reserved area lock */
	if (ReservedAreaIsLocked)
	{
		*error = ERROR_DISK_FULL;
		return FALSE;
	}

	return TRUE;
}


/* fileaccess that writes to a file
 */
BOOL CheckWriteAccess(fileentry_t *file, SIPTR *error, globaldata *g)
{

	if (!CheckChangeAccess(file, error, g))
		return FALSE;

	if (file->le.info.file.direntry->protection & FIBF_WRITE)
	{
		*error = ERROR_WRITE_PROTECTED;
		return FALSE;
	}

	return TRUE;
}


/* fileaccess that reads from a file
 */
BOOL CheckReadAccess(fileentry_t *file, SIPTR *error, globaldata *g)
{
	*error = 0;

	/* Test on read-protection, type and volume */
#if DELDIR
	if (!IsDelFile(file->le.info))
	{
#endif
		if (!IsFile(file->le.info)) 
		{
			*error = ERROR_OBJECT_WRONG_TYPE;
			return FALSE;
		}

		if (file->le.info.file.direntry->protection & FIBF_READ)
		{
			*error = ERROR_READ_PROTECTED;
			return FALSE;
		}
#if DELDIR
	}
#endif

	if (!CheckVolume(file->le.volume, 0, error, g))
		return FALSE;

	return TRUE;
}

/* check on operate access (like Seek)
 */
BOOL CheckOperateFile(fileentry_t *file, SIPTR *error, globaldata *g)
{
	*error = 0;

	/* test on type */
#if DELDIR
	if (!IsDelFile(file->le.info) && !IsFile(file->le.info))
#else
	if (!IsFile(file->le.info)) 
#endif
	{
		*error = ERROR_OBJECT_WRONG_TYPE;
		return FALSE;
	}

	/* volume must be or become currentvolume */
	if (!CheckVolume(file->le.volume, 0, error, g))
		return FALSE;

	return TRUE;
}


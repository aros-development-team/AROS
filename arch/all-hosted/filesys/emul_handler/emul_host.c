/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This file is just a dummy nonfunctional template. These functions
 * need host-specific implementations.
 */

#include <dos/dosasl.h>

#include "emul_intern.h"

LONG DoOpen(struct emulbase *emulbase, struct filehandle *fh, LONG mode, LONG protect, BOOL AllowDir)
{
    /* Open file or directory */
    return ERROR_NOT_IMPLEMENTED;
}

void DoClose(struct emulbase *emulbase, struct filehandle *fh)
{
    /* Close handle */
}

size_t DoRead(struct emulbase *emulbase, struct filehandle *fh, APTR buff, size_t len, SIPTR *err)
{
    /* Read from a file */
    *err = ERROR_NOT_IMPLEMENTED;
    return -1;
}

size_t DoWrite(struct emulbase *emulbase, struct filehandle *fh, CONST_APTR buff, size_t len, SIPTR *err)
{
    /* Write to a file */
    *err = ERROR_NOT_IMPLEMENTED;
    return -1;
}

off_t DoSeek(struct emulbase *emulbase, struct filehandle *fh, off_t Offset, ULONG Mode, SIPTR *err)
{
    /* Adjust file position */
    *err = ERROR_NOT_IMPLEMENTED;
    return -1;
}

LONG DoRewindDir(struct emulbase *emulbase, struct filehandle *fh)
{
    /* Reset directory search position */
    return ERROR_NOT_IMPLEMENTED;
}

LONG DoMkDir(struct emulbase *emulbase, struct filehandle *fh, ULONG protect)
{
    /* Create directory */
    return ERROR_NOT_IMPLEMENTED;
}

LONG DoDelete(struct emulbase *emulbase, char *name)
{
    /* Delete file or directory */
    return ERROR_NOT_IMPLEMENTED;
}

LONG DoChMod(struct emulbase *emulbase, char *filename, ULONG prot)
{
    /* Change protection bits */
    return ERROR_NOT_IMPLEMENTED;
}

LONG DoHardLink(struct emulbase *emulbase, char *fn, char *oldfile)
{
    /* Create a hard link */
    return ERROR_NOT_IMPLEMENTED;
}

LONG DoSymLink(struct emulbase *emulbase, char *dest, char *src)
{
    /* Create a symlink */
    return ERROR_NOT_IMPLEMENTED;
}

int DoReadLink(struct emulbase *emulbase, char *filename, char *buffer, ULONG size, LONG *err)
{
    /* Read a symlink */
    *err = ERROR_NOT_IMPLEMENTED;
    return -1;
}

LONG DoRename(struct emulbase *emulbase, char *filename, char *newfilename)
{
    /* Rename a file */
    return ERROR_NOT_IMPLEMENTED;
}

LONG DoSetDate(struct emulbase *emulbase, char *fullname, struct DateStamp *date)
{
    /* Set file date */
    return ERROR_NOT_IMPLEMENTED;
}

SIPTR DoSetSize(struct emulbase *emulbase, struct filehandle *fh, SIPTR offset, ULONG mode, SIPTR *err)
{
    /* Set file size */
    *err = ERROR_NOT_IMPLEMENTED;
    return -1;
}

LONG DoStatFS(struct emulbase *emulbase, char *path, struct InfoData *id)
{
    /* Get disk information */
    return ERROR_NOT_IMPLEMENTED;
}

LONG DoExamineEntry(struct emulbase *emulbase, struct filehandle *fh, char *name,
		   struct ExAllData *ead, ULONG size, ULONG type)
{
    /* Examine named object */
    return ERROR_NOT_IMPLEMENTED;
}

LONG examine_next(struct emulbase *emulbase,  struct filehandle *fh, struct FileInfoBlock *FIB)
{
    /* Examine next directory entry */
    return ERROR_NOT_IMPLEMENTED;
}

LONG examine_all(struct emulbase *emulbase, struct filehandle *fh, struct ExAllData *ead,
                  struct ExAllControl *eac, ULONG  size, ULONG  type)
{
    /* Examine all directory entries */
    return ERROR_NOT_IMPLEMENTED;
}

LONG examine_all_end(struct emulbase *emulbase, struct filehandle *fh)
{
    /* Finish directory search */
    return ERROR_NOT_IMPLEMENTED;
}

char *GetHomeDir(struct emulbase *emulbase, char *user)
{
    /* Get user's home directory */
    return NULL;
}

ULONG GetCurrentDir(struct emulbase *emulbase, char *path, ULONG len)
{
    /* Get AROS root directory */
    return 0;
}

BOOL CheckDir(struct emulbase *emulbase, char *name)
{
    /* Check if the directory is accessible */
    return FALSE;
}

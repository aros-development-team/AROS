/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This hook is called after dos.library wakes up.
 * Its job is to load all queued filesystems and add to the system.
 */

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/resident.h>
#include <resources/filesysres.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/partition.h>

#include "partition_support.h"
#include "fsloader.h"

static struct FileSysEntry *FindResidentFS(struct FileSysResource *fsr, ULONG dostype, ULONG version)
{
    struct FileSysEntry *fsrnode;

    ForeachNode(&fsr->fsr_FileSysEntries, fsrnode)
    {
    	if (fsrnode->fse_DosType == dostype && fsrnode->fse_Version >= version)
    	    return fsrnode;
    }

    return NULL;
}

ULONG AddFS(struct Library *PartitionBase, struct FileSysHandle *fs)
{
    struct DosLibrary *DOSBase = (struct DosLibrary *)((struct PartitionBase_intern *)PartitionBase)->pb_DOSBase;
    struct FileSysResource *fsr;
    struct FileSysEntry *fsrnode;
    ULONG dostype;
    ULONG version;

    fsr = OpenResource("FileSystem.resource");
    if (!fsr)
    	return ERROR_INVALID_RESIDENT_LIBRARY;

    GetFileSystemAttrs(&fs->ln, FST_ID, &dostype, FST_VERSION, &version, TAG_DONE);

    /*
     * First we want to check if we already have this filesystem in the resource.
     * Unfortunately the resource doesn't have any locking, so we have to use
     * Forbid()/Permit() pair. In order not to hold it for a while, we repeat
     * the check below after loading the handler (to eliminate race condition
     * when someone loads newer version of the filesystem that we are loading
     * at the moment.
     */
    Forbid();
    fsrnode = FindResidentFS(fsr, dostype, version);
    Permit();

    if (fsrnode)
    	return ERROR_OBJECT_EXISTS;

    fsrnode = AllocVec(sizeof(struct FileSysEntry), MEMF_PUBLIC | MEMF_CLEAR);
    if (!fsrnode)
        return ERROR_NO_FREE_STORE;

    GetFileSystemAttrs(&fs->ln, FST_FSENTRY, fsrnode, TAG_DONE);
    fsrnode->fse_SegList = LoadFileSystem(&fs->ln);

    /*
     * FIXME: Name of the filesystem is currently not filled in.
     * I left it this way just because this was originally done in m68k-specific hack.
     * May be it should be added?
     */

    if (fsrnode->fse_SegList)
    {
    	struct FileSysEntry *dup;

	/*
	 * Repeat checking, and insert the filesystem only if still not found.
	 * If found, unload our seglist and return error.
	 * This really sucks but nothing can be done with it. Even if we implement
	 * a global semaphore on the resource original m68k software won't know
	 * about it.
	 */
	Forbid();

	dup = FindResidentFS(fsr, dostype, version);
	if (!dup)
	    /*
	     * Entries in the list are not sorted by priority.
	     * Adding to head makes them sorted by version.
	     */
	    AddHead(&fsr->fsr_FileSysEntries, &fsrnode->fse_Node);

	Permit();

	if (dup)
	{
	    UnLoadSeg(fsrnode->fse_SegList);
	    FreeVec(fsrnode);

	    return ERROR_OBJECT_EXISTS;
	}

	return 0;
    }

    /* InternalLoadSeg() will leave its error code in IoErr() */
    return IoErr();
}

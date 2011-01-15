/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <proto/exec.h>

#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <resources/filesysres.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>

#include "os.h"
#include "afshandler.h"
#include "volumes.h"

#include LC_LIBDEFS_FILE

extern void AFS_work();
/* This is both a segment, and the data for the segment.
 * We will feed in to DOS/AddSegment() the BPTR to 
 * &aws_Next as the 'seglist' to add.
 */
struct AFS_work_Seg {
    ULONG              aws_Size;      /* Length of segment in # of ULONGs */
    ULONG              aws_Next;      /* Next segment (always 0 for this) */
    struct FullJumpVec aws_Code;      /* Code to jump to shell command */
};

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR afsbase)
{
    BOOL ok;
    APTR DOSBase;
    struct AFS_work_Seg *seg;
    struct FileSysResource *fsr;

    /* Create device node and add it to the system.
     * The handler will then be started when it is first accessed
     */
    DOSBase = OpenLibrary("dos.library", 0);
    if (DOSBase == NULL)
    	return FALSE;

    seg = AllocMem(sizeof(struct AFS_work_Seg), MEMF_CLEAR | MEMF_ANY);
    if (seg == NULL) {
    	CloseLibrary(DOSBase);
    	return FALSE;
    }

    afsbase->ab_Segment = seg;
    seg->aws_Size = 0;	// Must be 0 to prevent UnLoadSeg
    seg->aws_Next = 0;	// from killing us.
    __AROS_SET_FULLJMP(&seg->aws_Code, AFS_work);

    ok = AddSegment("afs.handler", MKBADDR(&seg->aws_Next), CMD_SYSTEM);
    if (ok) {
    	fsr = (struct FileSysResource*)OpenResource("FileSystem.resource");
    	if (fsr) {
    	    ULONG dos = 0x444f5300;
    	    UBYTE cnt;
    	    for (cnt = 1; cnt <= 7; cnt++) {
    	    	/* only fse_DosType, fse_Version and fse_PatchFlags needed */
    	    	struct FileSysEntry *fse = AllocMem(sizeof(struct Node) + sizeof(ULONG) * 3, MEMF_CLEAR);
    	    	if (fse) {
    	    	    fse->fse_DosType = dos + cnt;
    	    	    fse->fse_Version = (afsbase->ab_Lib.lib_Version << 16) | afsbase->ab_Lib.lib_Revision;
    	    	    AddTail(&fsr->fsr_FileSysEntries, fse);
    	    	}
    	    }
    	}
    } else {
    	FreeMem(seg, sizeof(struct AFS_work_Seg));
    }

    CloseLibrary(DOSBase);

    return ok;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)

/* NOTE: This is only here because architectures cannot
 *       override a libraries's *.conf file
 */
AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
           struct AFSBase *, afsbase, 5, Afs)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
           struct AFSBase *, afsbase, 6, Afs)
{
	AROS_LIBFUNC_INIT
	return 0;
	AROS_LIBFUNC_EXIT
}

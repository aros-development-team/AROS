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
#include <resources/filesysres.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

extern void AFS_work();

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR afsbase)
{
    struct FileSysResource *fsr;
    const ULONG dos = 0x444f5300;
    BPTR seg;
    int cnt;

    /* Create device node and add it to the system.
     * The handler will then be started when it is first accessed
     */
    fsr = (struct FileSysResource *)OpenResource("FileSystem.resource");
    if (fsr == NULL)
        return FALSE;

    seg = CreateSegList(AFS_work);
    if (seg == BNULL)
        return FALSE;

    for (cnt = 0; cnt <= 7; cnt++) {
	struct FileSysEntry *fse = AllocMem(sizeof(*fse), MEMF_CLEAR);
	if (fse) {
	    fse->fse_Node.ln_Name = "AFS/OFS";
	    fse->fse_Node.ln_Pri = 120 + cnt; /* Automount priority */
	    fse->fse_DosType = dos + cnt;
	    fse->fse_Version = (afsbase->lib_Version << 16) | afsbase->lib_Revision;
	    fse->fse_PatchFlags = FSEF_HANDLER | FSEF_SEGLIST | FSEF_GLOBALVEC;
	    fse->fse_Handler = AROS_CONST_BSTR("afs.handler");
	    fse->fse_SegList = seg;
	    fse->fse_GlobalVec = (BPTR)(SIPTR)-1;

	    /* Add to the list. I know forbid and permit are
	     * a little unnecessary for the pre-multitasking state
	     * we should be in at this point, but you never know
	     * who's going to blindly copy this code as an example.
	     */
    	    Forbid();
	    Enqueue(&fsr->fsr_FileSysEntries, (struct Node *)fse);
	    Permit();
	}
    }

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)

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

LONG mainprogram(struct ExecBase *SysBase);

void SFS_work(void)
{
    mainprogram(SysBase);
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR asfsbase)
{
    struct FileSysResource *fsr;
    struct FileSysEntry *fse;
    const ULONG sfs = AROS_MAKE_ID('S','F','S',0);

    /* Create device node and add it to the system.
     * The handler will then be started when it is first accessed
     */
    fsr = (struct FileSysResource *)OpenResource("FileSystem.resource");
    if (fsr == NULL)
    	return FALSE;

    asfsbase->ab_SegList = CreateSegList(SFS_work);
    if (asfsbase->ab_SegList == BNULL)
	return FALSE;

    fse = AllocMem(sizeof(*fse), MEMF_CLEAR);
    if (fse) {
	fse->fse_DosType = sfs;
	fse->fse_Version = (asfsbase->ab_Lib.lib_Version << 16) | asfsbase->ab_Lib.lib_Revision;
	fse->fse_PatchFlags = FSEF_SEGLIST | FSEF_HANDLER | FSEF_GLOBALVEC;
	fse->fse_SegList = asfsbase->ab_SegList;
	fse->fse_Handler = AROS_CONST_BSTR("sfs.handler");
	fse->fse_GlobalVec = (BPTR)(SIPTR)-1;
	Forbid();
	Enqueue(&fsr->fsr_FileSysEntries, (struct Node *)fse);
	Permit();
    }

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)

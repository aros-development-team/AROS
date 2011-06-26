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

LONG handler(struct ExecBase *SysBase);

void CDVDFS_work()
{
    handler(SysBase);
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR cdrombase)
{
    struct FileSysResource *fsr;
    struct FileSysEntry *fse;
    const ULONG sfs = AROS_MAKE_ID('A','C','D',0);

    /* Create device node and add it to the system.
     * The handler will then be started when it is first accessed
     */
    fsr = (struct FileSysResource *)OpenResource("FileSystem.resource");
    if (fsr == NULL)
    	return FALSE;

    cdrombase->ab_SegList = CreateSegList(CDVDFS_work);
    if (cdrombase->ab_SegList == BNULL)
	return FALSE;

    fse = AllocMem(sizeof(*fse), MEMF_CLEAR);
    if (fse) {
	fse->fse_DosType = sfs;
	fse->fse_Version = (cdrombase->ab_Lib.lib_Version << 16) | cdrombase->ab_Lib.lib_Revision;
	fse->fse_PatchFlags = FSEF_SEGLIST;
	fse->fse_SegList = cdrombase->ab_SegList;
	fse->fse_Handler = "cdrom.handler";
	AddTail(&fsr->fsr_FileSysEntries, (struct Node *)fse);
    }

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)

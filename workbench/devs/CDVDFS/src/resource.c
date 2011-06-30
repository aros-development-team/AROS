/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>

#include <resources/filesysres.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

LONG CDVD_handler(struct ExecBase *SysBase);

void CDVD_work()
{
    CDVD_handler(SysBase);
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR base)
{
    struct FileSysResource *fsr;
    struct FileSysEntry *fse;
    const ULONG dostype = AROS_MAKE_ID('C','D','V','D');
    BPTR seg;

    /* Create device node and add it to the system.
     * The handler will then be started when it is first accessed
     */
    fsr = (struct FileSysResource *)OpenResource("FileSystem.resource");
    if (fsr == NULL)
        return FALSE;

    seg = CreateSegList(CDVD_work);
    if (seg == BNULL)
        return FALSE;

    fse = AllocMem(sizeof(*fse), MEMF_CLEAR);
    if (fse) {
        fse->fse_Node.ln_Name = "ISO9660/HFS";
        fse->fse_Node.ln_Pri = 0;
        fse->fse_DosType = dostype;
        fse->fse_Version = (base->lib_Version << 16) | base->lib_Revision;
        fse->fse_PatchFlags = FSEF_HANDLER | FSEF_SEGLIST | FSEF_GLOBALVEC;
        fse->fse_Handler = AROS_CONST_BSTR("cdrom.handler");
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

    return TRUE;
}


ADD2INITLIB(GM_UNIQUENAME(Init),0)

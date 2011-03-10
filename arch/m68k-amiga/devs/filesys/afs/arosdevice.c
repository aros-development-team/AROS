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

#include "os.h"
#include "afshandler.h"
#include "volumes.h"

#include LC_LIBDEFS_FILE

extern void AFS_work();

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR afsbase)
{
    BOOL ok;
    struct FileSysResource *fsr;
    const ULONG dos = 0x444f5300;
    int cnt;

    /* Create device node and add it to the system.
     * The handler will then be started when it is first accessed
     */
    fsr = (struct FileSysResource *)OpenResource("FileSystem.resource");
    if (fsr == NULL)
    	return FALSE;

    afsbase->ab_SegList = CreateSegList(AFS_work);
    if (afsbase->ab_SegList == BNULL)
	return FALSE;

    for (cnt = 1; cnt <= 7; cnt++) {
	struct FileSysEntry *fse = AllocMem(sizeof(*fse), MEMF_CLEAR);
	if (fse) {
	    fse->fse_DosType = dos + cnt;
	    fse->fse_Version = (afsbase->ab_Lib.lib_Version << 16) | afsbase->ab_Lib.lib_Revision;
	    fse->fse_PatchFlags = FSEF_SEGLIST;
	    fse->fse_SegList = afsbase->ab_SegList;
	    AddTail(&fsr->fsr_FileSysEntries, fse);
	}
    }

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

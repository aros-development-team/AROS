/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <proto/exec.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <resources/filesysres.h>
#include <aros/asmcall.h>
#include <aros/debug.h>

#define HANDLER_NAME "afs.handler"
#define VERSION      41
#define REVISION     3

#define _STR(A) #A
#define STR(A) _STR(A)

extern const char afs_End;
static const TEXT version_string[];

static AROS_UFP3 (APTR, afs_Init,
		  AROS_UFPA(APTR, unused, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

const struct Resident afs_romtag =
{
   RTC_MATCHWORD,
   (struct Resident *)&afs_romtag,
   (APTR)&afs_End + 1,
   RTF_COLDSTART,
   VERSION,
   NT_PROCESS,
   -1,
   HANDLER_NAME,
   (STRPTR)version_string,
   afs_Init
};

static const TEXT version_string[] = HANDLER_NAME " " STR(VERSION) "." STR(REVISION) " (" ADATE ")\n";

extern void AFS_work();

static AROS_UFH3 (APTR, afs_Init,
		  AROS_UFHA(APTR, unused, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
   AROS_USERFUNC_INIT

    struct FileSysResource *fsr;
    const ULONG dos = 0x444f5300;
    BPTR seg;
    int cnt;

    /* Create device node and add it to the system.
     * The handler will then be started when it is first accessed
     */
    fsr = (struct FileSysResource *)OpenResource("FileSystem.resource");
    if (fsr == NULL)
        return NULL;

    seg = CreateSegList(AFS_work);
    if (seg == BNULL)
        return NULL;

    for (cnt = 0; cnt <= 7; cnt++) {
	struct FileSysEntry *fse = AllocMem(sizeof(*fse), MEMF_CLEAR);
	if (fse) {
	    fse->fse_Node.ln_Name = "AFS/OFS";
	    fse->fse_Node.ln_Pri = 120 + cnt; /* Automount priority */
	    fse->fse_DosType = dos + cnt;
	    fse->fse_Version = (VERSION << 16) | REVISION;
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

    return NULL;

    AROS_USERFUNC_EXIT
}

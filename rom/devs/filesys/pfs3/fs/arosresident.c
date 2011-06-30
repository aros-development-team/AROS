
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <resources/filesysres.h>
#include <proto/exec.h>

#include "versionhistory.doc"

#define _STR(A) #A
#define STR(A) _STR(A)

#define HANDLER_NAME "pfs3.handler"

extern void EntryPoint(void);

static AROS_UFP3 (APTR, Init,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT handler_name[] = HANDLER_NAME;
static const TEXT version_string[] =
   HANDLER_NAME " " STR(VERSION) "." STR(REVISION) " (" __DATE__ ")\n";

const struct Resident pfs3_tag =
{
   RTC_MATCHWORD,
   (struct Resident *)&pfs3_tag,
   (APTR)(&pfs3_tag + 1),
   RTF_COLDSTART,
   VERNUM,
   NT_UNKNOWN,
   -1,
   (STRPTR)handler_name,
   (STRPTR)version_string,
   (APTR)Init
};

static AROS_UFH3 (APTR, Init,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct FileSysResource *fsr;
    const ULONG pfs3 = 0x50465303;
    const ULONG pfs3ds = 0x50445303;
    BPTR seglist;
    WORD cnt;

    fsr = (struct FileSysResource *)OpenResource("FileSystem.resource");
    if (fsr == NULL)
    	return FALSE;

    seglist = CreateSegList(EntryPoint);
    if (seglist == BNULL)
	return FALSE;

    for (cnt = 0; cnt < 1; cnt++) {
	struct FileSysEntry *fse = AllocMem(sizeof(*fse), MEMF_CLEAR);
	if (fse) {
	    fse->fse_DosType = cnt == 0 ? pfs3 : pfs3ds;
	    fse->fse_Version = (VERNUM << 16) | REVNUM;
	    fse->fse_PatchFlags = FSEF_SEGLIST | FSEF_HANDLER | FSEF_GLOBALVEC;
	    fse->fse_SegList = seglist;
	    fse->fse_Handler = AROS_CONST_BSTR("pfs3.handler");
	    fse->fse_GlobalVec = (BPTR)(SIPTR)-1;
	    AddTail(&fsr->fsr_FileSysEntries, (struct Node *)fse);
	}
    }

    AROS_USERFUNC_EXIT

    return 0;
}

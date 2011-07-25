#include <libraries/iffparse.h>
#include <resources/filesysres.h>
#include <proto/exec.h>

ULONG __abox__ = 1;

int Start()
{
    struct ExecBase *SysBase = *(struct ExecBase **)(4L);
    struct FileSysResource *fsr = OpenResource("FileSystem.resource");

    if (fsr)
    {
	struct FileSysEntry *fse, *next;

	Forbid();

	for (fse = (struct FileSysEntry *)fsr->fsr_FileSysEntries.lh_Head;
	    fse->fse_Node.ln_Succ; fse = next)
	{
	    next = (struct FileSysEntry *)fse->fse_Node.ln_Succ;

	    if (fse->fse_DosType == MAKE_ID('C', 'D', 'F', 'S'))
	    {
		Remove(&fse->fse_Node);
	    }
	}

	Permit();
    }

    return 0;
}


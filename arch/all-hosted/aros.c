/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: This is the "boot code" of AROS when it runs as an emulation.
    Lang: english
*/
#include <dos/dostags.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <intuition/intuitionbase.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/exec.h>

extern struct DosLibrary *DOSBase;
struct IntuitionBase * IntuitionBase;

#define CANNOT_OPEN_INTUITION	"Cannot open intuition.library\n"
#define CANNOT_LOAD_SHELL	"Unable to load C:shell\n"
#define CANNOT_OPEN_CON		"Cannot open boot console\n"



int main(void)
{
    BPTR segs, cis = NULL, cos = NULL, ces = NULL, tmp;
    LONG rc;
    struct Process *me = (struct Process *)FindTask(NULL);


    /* Open a new console for the initial shell */
    if ((cis = Open("CON:20/20///Boot Shell/AUTO", FMF_READ)))
    {
	BPTR olddir = CurrentDir(cis);

	cos = Open("", FMF_WRITE);
	ces = Open("", FMF_WRITE);

	CurrentDir(olddir);
    }

    if (!(cis && cos && ces))
    {
	PutStr(CANNOT_OPEN_CON);
	rc = RETURN_FAIL;

	goto err1;
    }


    /* Use boot console for standard I/O. */
    cis = SelectInput(cis);
    cos = SelectOutput(cos);
    tmp = me->pr_CES; me->pr_CES = ces; ces = tmp;

    /* Load the boot shell
     * TODO: Switch to SystemTagList() when available
     */
    if ((segs = LoadSeg("C:shell")))
    {
	/* Execute it. */
	rc = RunCommand(segs, AROS_STACKSIZE, "FROM S:Startup-Sequence\n", -1);
	UnLoadSeg(segs);
    }
    else
    {
	PutStr(CANNOT_LOAD_SHELL);
	rc = RETURN_FAIL;
    }

    /* Restore original input/output */
    tmp = me->pr_CES; me->pr_CES = ces; ces = tmp;
    cos = SelectOutput(cos);
    cis = SelectInput(cis);

err1:
    Close(cos);
    Close(cis);
    Close(ces);

    return rc;
}

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: This is the "boot code" of AROS when it runs as an emulation.
    Lang: english
*/
#include <dos/dostags.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/exec.h>

extern struct DosLibrary *DOSBase;
struct IntuitionBase * IntuitionBase;

#define CANNOT_OPEN_INTUITION	"Cannot open intuition.library\n"
#define CANNOT_LOAD_SHELL	"Unable to load C:shell\n"
#define CANNOT_OPEN_CON		"Cannot open boot console\n"

#warning FIXME: for some reason opening the console here will crash the system 
#undef BOOT_ON_CONSOLE_WINDOW


int main(void)
{
	BPTR segs, in, out;
	LONG rc;

	if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
	{

#ifdef BOOT_ON_CONSOLE_WINDOW
		/* Open a new console for the initial shell
		 */
		if (!(in = Open("CON:////AROS Boot Shell/AUTO", MODE_READWRITE)))
		{
			/* No need to abort in case of failure: a process may live even
			 * with a NULL pr_COS/pr_CIS.
			 */
			PutStr(CANNOT_OPEN_CON);
		}

		out = in; /* FIXME: out = DupLock(in); */

		/* Use boot console for standard I/O. */
		in = SelectInput(in);
		out = SelectOutput(out);
		/* FIXME: what about pr_CES? */
#endif

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
			rc = 20;
		}

#ifdef BOOT_ON_CONSOLE_WINDOW
		/* Restore original input/output */
		SelectOutput(out);
		SelectInput(in);

		if (in)
			Close(in);

		/* FIXME:
			if (out)
				Close(out);
		 */
#endif

		CloseLibrary((struct Library *)IntuitionBase);
	}
	else
	{
		PutStr(CANNOT_OPEN_INTUITION);
		rc =20;
	}

	return rc;
} /* main */

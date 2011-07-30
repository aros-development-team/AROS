/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: Emergency console launcher for hosted AROS
    Lang: english
*/

#define DEBUG 0

#include <exec/alerts.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include <graphics/modeid.h>
#include <utility/tagitem.h>
#include <aros/debug.h>
#include <libraries/expansionbase.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include "emul_intern.h"

#undef GfxBase

/*****************************************************************************

    NAME */
#include <proto/emul.h>

AROS_LH0(void, EmulBoot,

/*  SYNOPSIS */

/*  LOCATION */
	struct emulbase *, EmulBase, 1, emul)

/*  FUNCTION
	Provides support for emergency console

    INPUTS
	None

    RESULT
	None

    NOTES
	This function is system-private. There's no need to call it from within
	any program.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase;
    struct DosLibrary *DOSBase;
    struct MsgPort *emulport;
    struct Process *me;
    struct FileHandle *fh_stdin, *fh_stdout;
    LONG rc;

     /*
     * This actually checks if we have at least one display mode in the database.
     * This means that we actually can open a display. If not, we enter emergency
     * shell on host's console.
     */
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 36);
    if (GfxBase)
    {
        ULONG displayid = NextDisplayInfo(INVALID_ID);

	CloseLibrary((APTR)GfxBase);
	if (displayid != INVALID_ID)
	{
	    /* Everything is ok, continue booting */
	    return;
	}
    }

    /*
     * Set up our own console I/O and run a shell without Startup-Sequence and other stuff.
     * A failure in the following code causes dos.library to continue booting.
     * Without display drivers this will end up in AN_SysScrn alert.
     */
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    if (!DOSBase)
    	return;

    emulport = DeviceProc("EMU");
    if (!emulport)
    	return;

    fh_stdin = AllocDosObject(DOS_FILEHANDLE, NULL);
    if (!fh_stdin)
    {
    	CloseLibrary(&DOSBase->dl_lib);
    	return;
    }

    fh_stdout = AllocDosObject(DOS_FILEHANDLE, NULL);
    if (!fh_stdout)
    {
    	CloseLibrary(&DOSBase->dl_lib);
    	return;
    }

    /* A little bit of magic */
    fh_stdin->fh_Interactive  = DOSTRUE;
    fh_stdin->fh_Type	      = emulport;
    fh_stdin->fh_Arg1	      = (SIPTR)EmulBase->eb_stdin;
    fh_stdout->fh_Interactive = DOSTRUE;
    fh_stdout->fh_Type	      = emulport;
    fh_stdout->fh_Arg1	      = (SIPTR)EmulBase->eb_stdout;

    SetVBuf(MKBADDR(fh_stdin) , NULL, BUF_LINE, -1);
    SetVBuf(MKBADDR(fh_stdout), NULL, BUF_LINE, -1);

    if (Input())
    	Close(Input());
    if (Output())
    	Close(Output());

    D(bug("[EmulBoot] Selecting input and output for DOS\n"));

    SelectInput(MKBADDR(fh_stdin));
    SelectOutput(MKBADDR(fh_stdout));
    me = (struct Process *)FindTask(NULL);
    me->pr_CES = MKBADDR(fh_stdout);

    PutStr("Failed to load system HIDDs. Entering emergency shell.\n"
           "EndCLI will reboot the system.\n");

    rc = SystemTags("", SYS_Asynch, TRUE, SYS_Background, FALSE, TAG_DONE);
    if (rc == -1)
    {
    	/* Everything is too bad */
        PutStr("Cannot open boot console. System halted.\n");
        Wait(0);
    }

    ColdReboot();

    AROS_LIBFUNC_EXIT
}

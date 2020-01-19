/*
    Copyright  1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Emergency console launcher for hosted AROS
    Lang: english
*/

#define DEBUG 0

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include <exec/resident.h>
#include <graphics/modeid.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include "emul_intern.h"

#define EXPANSION_NOPRIVATEBASES
#include "expansion_intern.h"
#include "bootflags.h"

#undef GfxBase

static const UBYTE version[];
extern const char emul_End;

AROS_UFP3S(APTR, EmulBoot,
	   AROS_UFPA(ULONG, dummy, D0),
	   AROS_UFPA(BPTR, segList, A0),
	   AROS_UFPA(struct ExecBase *, SysBase, A6));

__section(".text.romtag") const struct Resident EmulBoot_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&EmulBoot_resident,
    (APTR)&emul_End,
    RTF_AFTERDOS,
    2,
    NT_PROCESS,
    -128,
    "Emergency console",
    (STRPTR)&version[6],
    EmulBoot
};

__section(".text.romtag") static const UBYTE version[] = "$VER: emul-handler emergency console v2.0";

AROS_UFH3(APTR, EmulBoot,
	   AROS_UFPA(ULONG, dummy, D0),
	   AROS_UFPA(BPTR, segList, A0),
	   AROS_UFPA(struct ExecBase *, SysBase, A6));
{
    AROS_USERFUNC_INIT

    struct emulbase *EmulBase;
    struct GfxBase *GfxBase;
    struct DosLibrary *DOSBase;
    struct Library *ExpansionBase;
    struct MsgPort *emulport;
    struct Process *me;
    struct FileHandle *fh_stdin, *fh_stdout;
    LONG rc;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    D(bug("[EmulBoot] DOSbase @ 0x%p\n", DOSBase);)

    ExpansionBase = OpenLibrary("expansion.library", 0);
    if (ExpansionBase)
    {
        ULONG BootFlags = IntExpBase(ExpansionBase)->BootFlags;
        D(bug("[EmulBoot] Expansionbase @ 0x%p\n", ExpansionBase);)

        if (!(BootFlags & BF_NO_DISPLAY_DRIVERS))
        {
            if (DOSBase)
            {
                BPTR seg = LoadSeg("C:AROSMonDrvs");
                D(bug("[EmulBoot] AROSMonDrvs Seglist @ 0x%p\n", seg);)

                if (seg != BNULL)
                {
                    STRPTR args = "NOCOMPOSITION";
                    BPTR oldin, oldout;

                    D(bug("[EmulBoot] loading display drivers...\n");)

                    oldin = SelectInput(Open("NIL:", MODE_OLDFILE));
                    oldout= SelectOutput(Open("NIL:", MODE_NEWFILE));
                    RunCommand(seg, AROS_STACKSIZE, args, strlen(args));
                    SelectInput(oldin);
                    SelectOutput(oldout);

                    /* We don't care about the return code */
                    UnLoadSeg(seg);
                }
                /* make sure the boot process doesnt try to load the drivers again.. */
                IntExpBase(ExpansionBase)->BootFlags = BootFlags | BF_NO_DISPLAY_DRIVERS;
            }
        }
        CloseLibrary(ExpansionBase);
    }

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
            if (DOSBase)
                CloseLibrary((APTR)DOSBase);
	    /* Everything is ok, continue booting */
	    return NULL;
	}
    }

    /*
     * Set up our own console I/O and run a shell without Startup-Sequence and other stuff.
     * A failure in the following code causes dos.library to continue booting.
     * Without display drivers this will end up in AN_SysScrn alert.
     */
    if (!DOSBase)
    	return NULL;

    EmulBase = OpenResource("emul-handler");
    if (!EmulBase)
	return NULL;

    emulport = DeviceProc("EMU");
    if (!emulport)
    	return NULL;

    fh_stdin = AllocDosObject(DOS_FILEHANDLE, NULL);
    if (!fh_stdin)
    {
    	CloseLibrary(&DOSBase->dl_lib);
    	return NULL;
    }

    fh_stdout = AllocDosObject(DOS_FILEHANDLE, NULL);
    if (!fh_stdout)
    {
    	CloseLibrary(&DOSBase->dl_lib);
    	return NULL;
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

    Close(Input());
    Close(Output());

    D(bug("[EmulBoot] Selecting input and output for DOS\n"));

    SelectInput(MKBADDR(fh_stdin));
    SelectOutput(MKBADDR(fh_stdout));
    me = (struct Process *)FindTask(NULL);
    me->pr_CES = MKBADDR(fh_stdout);

    SetPrompt("%N> ");

    PutStr("Display driver(s) failed to initialize. Entering emergency shell.\n"
    	   "EndCLI will quit AROS.\n");

    rc = SystemTags("", SYS_Asynch, FALSE, SYS_Background, FALSE, TAG_DONE);
    if (rc == -1)
    {
    	/* Everything is too bad */
        PutStr("Cannot open boot console. System halted.\n");
        RemTask(NULL);
    }

    ShutdownA(SD_ACTION_POWEROFF);

    return NULL; /* Just shut up the compiler, we'll never get here */

    AROS_USERFUNC_EXIT
}

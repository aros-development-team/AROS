/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot your operating system.
    Lang: english
*/

#define DEBUG 0

#include <exec/alerts.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>
#include <graphics/modeid.h>
#include <utility/tagitem.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include "dosboot_intern.h"

#undef GfxBase

struct emulbase
{
    struct Device eb_device;
    APTR eb_stdin;
    APTR eb_stdout;
    APTR eb_stderr;
};

void __dosboot_Boot(APTR BootLoaderBase, struct DosLibrary *DOSBase, ULONG Flags)
{
    /*  We have been created as a process by DOS, we should now
    	try and boot the system. We do this by calling the submain()
    	function in arosshell.c

    	DOS has created our process, but it has not given us
    	the correct I/O Streams yet. What we have to do is
    	to (conditionally) close the old streams, and set
    	our own, which we get from emul.handler.

    	This is really only necessary if
    	a) We want to use the host console as our boot shell
    	b) Don't have a working console.device/CON: handler.
    */

    struct GfxBase *GfxBase;
    struct emulbase *emulbase;
    struct TagItem fhtags[]= { { TAG_END, 0 } };
    struct FileHandle *fh_stdin, *fh_stdout;
    struct TagItem tags[] =
    {
        { SYS_Asynch,      TRUE       }, /* 0 */
        { SYS_Background,  FALSE      }, /* 1 */
        { SYS_ScriptInput, 0          }, /* 2 */
        { SYS_Input,       0          }, /* 3 */
        { SYS_Output,      0          }, /* 4 */
        { SYS_Error,       0          }, /* 5 */
        { TAG_DONE,        0          }
    };
    BPTR cis = 0;
    BPTR sseq = 0;
    LONG rc = RETURN_FAIL;
    BOOL hidds_ok = FALSE;
    struct Process *me;

    D(bug("[DOSBoot.hosted] __dosboot_Boot()\n")); 

    /*
	This is quite naughty, but I know what I'm doing here, since
	emul.handler ALWAYS exists under Unix, and it won't go away.
    */
    Forbid();
    emulbase = (struct emulbase *)FindName(&SysBase->DeviceList, "emul.handler");
    Permit();
    D(bug("[DOSBoot.hosted] __dosboot_Boot: emulbase = 0x%08lX\n", emulbase));

    if( emulbase == NULL )
    {
	/* BootStrap couldn't open unknown */
	Alert(AT_DeadEnd | AN_BootStrap | AG_OpenDev | AO_Unknown );
    }

    fh_stdin = AllocDosObject(DOS_FILEHANDLE, fhtags);
    fh_stdout = AllocDosObject(DOS_FILEHANDLE, fhtags);

    if(fh_stdin == NULL || fh_stdout == NULL)
    {
    	/* We have got some problems here. */
    	Alert(AT_DeadEnd | AN_BootStrap | AG_NoMemory);
    }

    fh_stdin->fh_Device  = &emulbase->eb_device;
    fh_stdin->fh_Unit    = emulbase->eb_stdin;
    fh_stdout->fh_Device = &emulbase->eb_device;
    fh_stdout->fh_Unit   = emulbase->eb_stdout;

    SetVBuf(MKBADDR(fh_stdin) , NULL, BUF_LINE, -1);
    SetVBuf(MKBADDR(fh_stdout), NULL, BUF_LINE, -1);

    if (Input())
    	Close(Input());
    if (Output())
    	Close(Output());

    D(bug("[DOSBoot.hosted] __dosboot_Boot: Selecting input and output for DOS\n"));
    SelectInput(MKBADDR(fh_stdin));
    SelectOutput(MKBADDR(fh_stdout));
    me = (struct Process *)FindTask(NULL);
    me->pr_CES = MKBADDR(fh_stdout);

    D(bug("[DOSBoot.hosted] __dosboot_Boot: Selecting output for AROSSupport\n"));
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->StdOut = fh_stdout;

    /*
     * This actually checks if we have at least one display mode in the database.
     * This means that we actually can open a display. If not, we enter emergency
     * shell on host's console.
     */
    GfxBase = OpenLibrary("graphics.library", 36);
    if (GfxBase)
    {
        if (NextDisplayInfo(INVALID_ID) != INVALID_ID)
	    hidds_ok = TRUE;
	CloseLibrary(&GfxBase->LibNode);
    }

    if (hidds_ok)
    {
        D(bug("[DOSBoot.hosted] __dosboot_Boot: Opening boot shell\n"));
        cis  = Open("CON:20/20///Boot Shell/AUTO", MODE_OLDFILE);
    } else
        PutStr("Failed to load system HIDDs\n");
    if (cis)
    {
        D(bug("[DOSBoot.hosted] __dosboot_Boot: Flags = 0x%08lX\n", Flags));

        if (!(Flags & BF_NO_STARTUP_SEQUENCE))
        {
            sseq = Open("S:Startup-Sequence", MODE_OLDFILE);
            tags[2].ti_Data = (IPTR)sseq;
        }
        tags[3].ti_Data = (IPTR)cis;
    } else {
        tags[3].ti_Tag = TAG_DONE;
        PutStr("Entering emergency shell\n");
    }
    rc = SystemTagList("", tags);
    if (rc != -1)
    {
        cis  = NULL;
        sseq = NULL;
    }
    else {
        PutStr("Cannot open boot console\n");
        rc = RETURN_FAIL;
    }
    if (sseq)
        Close(sseq);
    if (cis)
        Close(cis);

    /* We get here when the Boot Shell Window is left with EndShell/EndCli.
       To avoid that the input/output/error handles of emul.handler
       are closed when the Boot Process dies, we set the in/out/err
       handles of this process to 0.
    */

    SelectInput(0);
    SelectOutput(0);
    me->pr_CES = 0;

    /* No RemTask() here, otherwise the process cleanup routines
       are not called. And that would for example mean, that the
       Boot Process (having a CLI) is not removed from the rootnode.
       --> Dead stuff in there -> Crash
    */
}

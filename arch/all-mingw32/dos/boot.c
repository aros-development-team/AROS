/*
    Copyright  1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot your operating system.
    Lang: english
*/
#define DEBUG 0

#include <exec/types.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/stdio.h>
#include <libraries/expansionbase.h>
#include <utility/tagitem.h>
#include <aros/arossupportbase.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

struct emulbase
{
    struct Device eb_device;
    APTR eb_stdin;
    APTR eb_stdout;
    APTR eb_stderr;
};

void boot(struct ExecBase *SysBase, BOOL hidds_ok)
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

    struct DosLibrary *DOSBase;
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
    BPTR cis = NULL;
    BPTR sseq = NULL;
    LONG rc = RETURN_FAIL;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if( DOSBase == NULL )
    {
    	/* BootStrap couldn't open dos.library */
    	Alert(AT_DeadEnd | AN_BootStrap | AG_OpenLib | AO_DOSLib );
    }

    /*
	This is quite naughty, but I know what I'm doing here, since
	emul.handler ALWAYS exists under Unix, and it won't go away.
    */
    Forbid();
    emulbase = (struct emulbase *)FindName(&SysBase->DeviceList, "emul.handler");
    Permit();
    D(bug("[boot] emulbase = 0x%08lX\n", emulbase));

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

    fh_stdin->fh_Device  =&emulbase->eb_device;
    fh_stdin->fh_Unit    =emulbase->eb_stdin;
    fh_stdout->fh_Device =&emulbase->eb_device;
    fh_stdout->fh_Unit   =emulbase->eb_stdout;
    SetVBuf(fh_stdin, NULL, BUF_LINE, -1);
    SetVBuf(fh_stdout, NULL, BUF_LINE, -1);

    if(Input())
    	Close(Input());
    if(Output())
    	Close(Output());

    D(bug("[boot] Selecting input and output for DOS\n"));
    SelectInput(MKBADDR(fh_stdin));
    SelectOutput(MKBADDR(fh_stdout));
    SelectError(MKBADDR(fh_stdout));

    D(bug("[boot] Selecting output for AROSSupport\n"));
    ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->StdOut = fh_stdout;

    if (hidds_ok) {
        D(bug("[SubMain] Opening boot shell\n"));
        cis  = Open("CON:20/20///Boot Shell/AUTO", FMF_READ);
    } else
        PutStr("Failed to load system HIDDs\n");
    if (cis)
    {
        struct ExpansionBase *ExpansionBase;
        BOOL opensseq = TRUE;

	D(bug("[SubMain] Boot shell opened\n"));
        if ((ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0)) != NULL)
        {
            opensseq = !(ExpansionBase->Flags & EBF_DOSFLAG);
            CloseLibrary(ExpansionBase);
        }

        D(bug("[SubMain] Open Startup Sequence = %d\n", opensseq));

        if (opensseq)
        {
            sseq = Open("S:Startup-Sequence", FMF_READ);
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
    ((struct Process *)FindTask(NULL))->pr_CES = 0;

    /* No RemTask() here, otherwise the process cleanup routines
       are not called. And that would for example mean, that the
       Boot Process (having a CLI) is not removed from the rootnode.
       --> Dead stuff in there -> Crash
    */
}

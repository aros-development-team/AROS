/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Boot your operating system.
    Lang: english
*/

#include <exec/types.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <aros/arossupportbase.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

/* Require this for the stdout defn */
#include <stdio.h>

extern void AROSSupportBase_SetStdOut (void *);
extern int submain(struct ExecBase *, struct DosLibrary *);

struct emulbase
{
    struct Device eb_device;
    struct Unit *eb_stdin;
    struct Unit *eb_stdout;
    struct Unit *eb_stderr;
};

AROS_UFH3(void, boot,
    AROS_UFHA(STRPTR, argString, A0),
    AROS_UFHA(ULONG, argSize, D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    /*  We have been created as a process by DOS, we should now
    	try and boot the system. We do this by calling the submain()
    	function in arosshell.c
    	
    	DOS has created our process, but it has not given us
    	the correct I/O Streams yet. What we have to do is
    	to (conditionally) close the old streams, and set
    	our own, which we get from emul.handler.

    	This is really only necessary if 
    	a) We want to use the XTerm as our boot shell
    	b) Don't have a working console.device/CON: handler.
    */

    AROS_USERFUNC_INIT

    struct DosLibrary *DOSBase;
//    struct emulbase *emulbase;
//    struct TagItem fhtags[]= { { TAG_END, 0 } };
//    struct FileHandle *fh_stdin, *fh_stdout;

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

/*
    Forbid();
    emulbase = (struct emulbase *)FindName(&SysBase->DeviceList, "emul.handler");
    Permit();

    if( emulbase == NULL )
    {
*/
	/* BootStrap couldn't open unknown */
/*
	Alert(AT_DeadEnd | AN_BootStrap | AG_OpenDev | AO_Unknown );
    }

    fh_stdin = AllocDosObject(DOS_FILEHANDLE, fhtags);
    fh_stdout = AllocDosObject(DOS_FILEHANDLE, fhtags);
 
    if(fh_stdin == NULL || fh_stdout == NULL)
    {
*/
    	/* We have got some problems here. */
/*    	Alert(AT_DeadEnd | AN_BootStrap | AG_NoMemory);
    }
 
    fh_stdin->fh_Device  =&emulbase->eb_device;
    fh_stdin->fh_Unit    =emulbase->eb_stdin;
    fh_stdout->fh_Device =&emulbase->eb_device;
    fh_stdout->fh_Unit   =emulbase->eb_stdout;
 
    if(Input())
    	Close(Input());
    if(Output())
    	Close(Output());
 
    SelectInput(MKBADDR(fh_stdin));
    SelectOutput(MKBADDR(fh_stdout));
    ((struct Process *)FindTask(NULL))->pr_CES = MKBADDR(fh_stdout);
 
    AROSSupportBase_SetStdOut (stderr);
*/ 
    submain(SysBase, DOSBase);

    /* submain() returns, when the Boot Shell Window is left with EndShell/EndCli */
    
    /* To avoid that the input/output/error handles of emul.handler
       are closed when the Boot Process dies, we set the in/out/err
       handles of this process to 0.
    */

/* 
    SelectInput(0);
    SelectOutput(0);
    ((struct Process *)FindTask(NULL))->pr_CES = 0; 
*/

    /* No RemTask() here, otherwise the process cleanup routines
       are not called. And that would for example mean, that the
       Boot Process (having a CLI) is not removed from the rootnode.
       --> Dead stuff in there -> Crash
    */
    AROS_USERFUNC_EXIT
}

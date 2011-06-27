/*
 Copyright © 1995-2010, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: Filesystem that accesses an underlying host OS filesystem.
       Low-level host-dependent subroutines for Windows.
 Lang: english
 */

/*********************************************************************************************/

#define DEBUG 0
#define DERROR(x)
#define DEXAM(x)
#define DFSIZE(x)
#define DLINK(x)
#define DLOCK(x)
#define DOPEN(x)
#define DOPEN2(x)
#define DSEEK(x)
#define DSTATFS(x)
#define DASYNC(x)

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/system.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <dos/bptr.h>
#include <proto/arossupport.h>
#include <proto/dos.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>

#include <limits.h>
#include <string.h>

#include "emul_intern.h"

/*********************************************************************************************/

static void EmulIntHandler(struct AsyncReaderControl *msg, void *d)
{
    DASYNC(bug("[emul] Interrupt on request 0x%p, task 0x%p, signal 0x%08lX\n", msg, msg->task, msg->sig));
    Signal(msg->task, msg->sig);
}

/*********************************************************************************************/

static struct filehandle *CreateStdHandle(struct emulbase *emulbase, ULONG id)
{
    struct filehandle *fh;
    APTR handle;

    Forbid();
    handle = emulbase->pdata.KernelIFace->GetStdHandle(id);
    Permit();

    if (!handle)
	return NULL;

    fh = AllocMem(sizeof(struct filehandle), MEMF_PUBLIC|MEMF_CLEAR);
    if (fh) {
	fh->type = FHD_FILE|FHD_STDIO;
	fh->fd   = handle;
    }

    return fh;
}

/*********************************************************************************************/

const char *EmulSymbols[] = {
    "Emul_Init_Native",
    "EmulGetHome",
    NULL
};
    
const char *KernelSymbols[] = {
    "CreateFileA",
    "CloseHandle",
    "ReadFile",
    "WriteFile",
    "SetFilePointer",
    "SetEndOfFile",
    "GetFileType",
    "GetStdHandle",
    "MoveFileA",
    "GetCurrentDirectoryA",
    "FindFirstFileA",
    "FindNextFileA",
    "FindClose",
    "CreateDirectoryA",
    "SetFileAttributesA",
    "GetLastError",
    "CreateHardLinkA",
    "CreateSymbolicLinkA",
    "SetEvent",
    "SetFileTime",
    "GetFileAttributesA",
    "GetFileAttributesExA",
    "DeleteFileA",
    "RemoveDirectoryA",
    "GetDiskFreeSpaceA",
    NULL
};

static LONG host_startup(struct emulbase *emulbase)
{
    ULONG r;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[EmulHandler] got hostlib.resource %p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
	return FALSE;

    emulbase->pdata.EmulHandle = HostLib_Open("Libs\\Host\\emul_handler.dll", NULL);
    if (!emulbase->pdata.EmulHandle) {
	D(bug("[EmulHandler] Unable to open emul.handler host-side library!\n"));
	return FALSE;
    }

    emulbase->pdata.EmulIFace = (struct EmulInterface *)HostLib_GetInterface(emulbase->pdata.EmulHandle, EmulSymbols, &r);
    D(bug("[EmulHandler] Native library interface: 0x%08lX\n", emulbase->pdata.EmulIFace));
    if ((!emulbase->pdata.EmulIFace) || r)
	return FALSE;

    emulbase->pdata.KernelHandle = HostLib_Open("kernel32.dll", NULL);
    if (!emulbase->pdata.KernelHandle)
	return FALSE;

    emulbase->pdata.KernelIFace = (struct KernelInterface *)HostLib_GetInterface(emulbase->pdata.KernelHandle, KernelSymbols, &r);
    if (!emulbase->pdata.KernelIFace)
	return FALSE;

    D(bug("[EmulHandler] %lu unresolved symbols in kernel32.dll\n", r));
    D(bug("[EmulHandler] CreateHardLink()     : 0x%08lX\n", emulbase->pdata.KernelIFace->CreateHardLink));
    D(bug("[EmulHandler] CreateSymbolicLink() : 0x%08lX\n", emulbase->pdata.KernelIFace->CreateSymbolicLink));
    if (r > 2)
	return FALSE;

    D(bug("[Emulhandler] Creating console reader\n"));
    Forbid();
    emulbase->pdata.ConsoleReader = emulbase->pdata.EmulIFace->EmulInitNative();
    Permit();

    D(bug("[Emulhandler] Console reader at %p\n", emulbase->pdata.ConsoleReader));
    if (!emulbase->pdata.ConsoleReader)
	return FALSE;
	
    D(bug("[Emulhandler] Console reader IRQ %u\n", emulbase->pdata.ConsoleReader->IrqNum));
    emulbase->ReadIRQ = KrnAddIRQHandler(emulbase->pdata.ConsoleReader->IrqNum, EmulIntHandler, emulbase->pdata.ConsoleReader, NULL);
    D(bug("[Emulhandler] Added console interrupt %p\n", emulbase->ReadIRQ));
    if (!emulbase->ReadIRQ)
	return FALSE;

    emulbase->eb_stdin  = CreateStdHandle(emulbase, STD_INPUT_HANDLE);
    emulbase->eb_stdout = CreateStdHandle(emulbase, STD_OUTPUT_HANDLE);
    emulbase->eb_stderr = CreateStdHandle(emulbase, STD_ERROR_HANDLE);

    return TRUE;
}

ADD2INITLIB(host_startup, 0);

static int host_cleanup(struct emulbase *emulbase)
{
    D(bug("[EmulHandler] Expunge\n"));
    
    if (KernelBase)
    {
	/* TODO: shut down console reader */
	if (emulbase->ReadIRQ)
	    KrnRemIRQHandler(emulbase->ReadIRQ);
    }

    if (!HostLibBase)
    	return TRUE;

    if (emulbase->pdata.KernelIFace)
    	HostLib_DropInterface((APTR *)emulbase->pdata.KernelIFace);

    if (emulbase->pdata.EmulIFace)
    	HostLib_DropInterface((APTR *)emulbase->pdata.EmulIFace);

    if (emulbase->pdata.EmulHandle)
    	HostLib_Close(emulbase->pdata.EmulHandle, NULL);

    if (emulbase->pdata.KernelHandle)
    	HostLib_Close(emulbase->pdata.KernelHandle, NULL);

    return TRUE;
}

ADD2EXPUNGELIB(host_cleanup, 0);

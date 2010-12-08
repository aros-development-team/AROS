/*
 Copyright � 1995-2010, The AROS Development Team. All rights reserved.
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

/* Make Windows protection bits out of AROS protection bits. */
static ULONG prot_a2w(ULONG protect)
{
  ULONG uprot = 0;
  
  /* The following flags are low-active! */
  if ((protect & (FIBF_WRITE|FIBF_DELETE)) == (FIBF_WRITE|FIBF_DELETE))
	uprot = FILE_ATTRIBUTE_READONLY;
  /* The following flags are high-active again. */
  if (protect & FIBF_ARCHIVE)
      	uprot |= FILE_ATTRIBUTE_ARCHIVE;
  if (protect & FIBF_SCRIPT)
	uprot |= FILE_ATTRIBUTE_SYSTEM;

  /* TODO: 1) Support more NT-specific attributes ('Executable')
           2) Write complete AROS protection bits support using NTFS streams */
  
  return uprot;
}

/*********************************************************************************************/

/* Make AROS protection bits out of Windows protection bits. */
static ULONG prot_w2a(ULONG protect)
{
  ULONG uprot = 0;
  
  /* The following three flags are low-active! */
  if (protect & FILE_ATTRIBUTE_READONLY)
      uprot = FIBF_WRITE|FIBF_DELETE;
  if (protect & FILE_ATTRIBUTE_DIRECTORY)
      uprot |= FIBF_EXECUTE;
  /* The following flags are high-active again. */
  if (protect & FILE_ATTRIBUTE_ARCHIVE)
      uprot |= FIBF_ARCHIVE;
  if (protect & FILE_ATTRIBUTE_SYSTEM)
      uprot |= FIBF_SCRIPT;

  /* TODO: 1) Support more NT-specific attributes ('Executable')
           2) Write complete AROS protection bits support using NTFS streams */

  return uprot;
}

/*********************************************************************************************/

static void FileTime2DateStamp(struct DateStamp *ds, UQUAD ft)
{
    UQUAD totalmins;

    /* Adjust from 01.01.1601 to 01.01.1978. This offset was calculated using a specially written program
       which puts "01.01.1978 00:00:00" into SYSTEMTIME structure and converts it into FILETIME. */
    ft -= 118969344000000000LL;
    /* Adjust interval from 100 ns to 1/50 sec */
    ft /= 200000;
    totalmins = ft / (60*50);
    ds->ds_Days = totalmins / (24*60);
    ds->ds_Minute = totalmins % (24*60);
    ds->ds_Tick = ft % (60*50);
}

/*********************************************************************************************/

static UQUAD DateStamp2FileTime(struct DateStamp *ds)
{
    UQUAD ticks;

    /* Get total number of minutes */
    ticks = ds->ds_Days * (24*60) + ds->ds_Minute;
    /* Convert to ticks and add ds_Tick */
    ticks = ticks * (60*50) + ds->ds_Tick;

    return ticks * 200000 + 118969344000000000LL;
}

/*********************************************************************************************/

/* Make an AROS error-code (<dos/dos.h>) out of a Windows error-code. */
static ULONG u2a[][2]=
{
  { ERROR_PATH_NOT_FOUND	, ERROR_OBJECT_NOT_FOUND	},
  { ERROR_NO_MORE_FILES		, ERROR_NO_MORE_ENTRIES		},
  { ERROR_NOT_ENOUGH_MEMORY	, ERROR_NO_FREE_STORE		},
  { ERROR_FILE_NOT_FOUND	, ERROR_OBJECT_NOT_FOUND	},
  { ERROR_FILE_EXISTS		, ERROR_OBJECT_EXISTS		},
  { ERROR_WRITE_PROTECT		, ERROR_WRITE_PROTECTED		},
  { WIN32_ERROR_DISK_FULL	, ERROR_DISK_FULL		},
  { ERROR_DIR_NOT_EMPTY		, ERROR_DIRECTORY_NOT_EMPTY	},
  { ERROR_SHARING_VIOLATION	, ERROR_OBJECT_IN_USE		},
  { ERROR_LOCK_VIOLATION	, ERROR_OBJECT_IN_USE		},
  { WIN32_ERROR_BUFFER_OVERFLOW	, ERROR_OBJECT_TOO_LARGE	},
  { ERROR_INVALID_NAME		, ERROR_OBJECT_NOT_FOUND	},
  { ERROR_HANDLE_EOF		, 0				},
  { 0				, 0				}
};

static ULONG Errno_w2a(ULONG e, LONG mode)
{
    ULONG i;

    DERROR(printf("[EmulHandler] Windows error code: %lu\n", e));

    /* ERROR_ACCESS_DENIED may mean different AmigaDOS errors depending
       on the context */
    if (e == ERROR_ACCESS_DENIED)
    {
	if (mode == FMF_MODE_OLDFILE)
	    return ERROR_READ_PROTECTED;
	
	if (mode & FMF_WRITE)
	    return ERROR_WRITE_PROTECTED;

	if (mode == 0)
	    return ERROR_DELETE_PROTECTED;

	return ERROR_READ_PROTECTED;
    }

    for(i=0;i<sizeof(u2a)/sizeof(u2a[0]);i++)
    {
	if(u2a[i][0]==e) {
	    DERROR(printf("[EmulHandler] Translated to AROS error code: %lu\n", u2a[i][1]));
	    return u2a[i][1];
	}
    }

    DERROR(printf("[EmulHandler] Unknown error code\n"));
    return ERROR_UNKNOWN;
}

/*********************************************************************************************/

/* Free a filehandle */
void DoClose(struct emulbase *emulbase, struct filehandle *current)
{
    DLOCK(bug("[emul] Lock type = %lu\n", current->type));
    switch(current->type)
    {
    case FHD_FILE:
	/* Nothing will happen if type has FHD_STDIO set, this is intentional */
	DLOCK(bug("[emul] CloseHandle(), fd = 0x%08lX\n", current->fd));

	Forbid();
	emulbase->pdata.KernelIFace->CloseHandle(current->fd);
	Permit();
	break;

    case FHD_DIRECTORY:
	if (current->fd != INVALID_HANDLE_VALUE)
	{
	    DLOCK(bug("[emul] Closing directory search handle\n"));
	    Forbid();
	    emulbase->pdata.KernelIFace->FindClose(current->fd);
	    Permit();
	}
	
	if (current->ph.pathname)
	{
	    DLOCK(bug("[emul] Freeing directory search path\n"));
	    FreeVecPooled(emulbase->mempool, current->ph.pathname);
	}
	break;
    }

    DLOCK(bug("[emul] Done\n"));
}

/*********************************************************************************************/

LONG DoOpen(struct emulbase *emulbase, struct filehandle *fh, LONG mode, LONG protect, BOOL AllowDir)
{
    ULONG kind;

    DOPEN(bug("[emul] DoOpen(), directories allowed: %lu\n", AllowDir));
    DOPEN(bug("[emul] AROS object name: %s, host object name: %s\n", fh->name, fh->hostname));

    Forbid();
    kind = emulbase->pdata.KernelIFace->GetFileAttributes(fh->hostname);
    Permit();

    DOPEN(bug("[emul] Object attributes: 0x%08X\n", kind));

    /* Non-existing objects can be files opened for writing */
    if (kind == INVALID_FILE_ATTRIBUTES)
	kind = 0;

    if (kind & FILE_ATTRIBUTE_DIRECTORY)
    {
    	/* file is a directory */
	if (!AllowDir)
	    return ERROR_OBJECT_WRONG_TYPE;

	fh->type   = FHD_DIRECTORY;
	fh->fd     = INVALID_HANDLE_VALUE;
	fh->ph.dirpos = 0;
    }
    else
    {
	ULONG flags = 0;
	ULONG lock, create, err;

	DOPEN2(bug("[emul] Open file \"%s\", mode 0x%08lX\n", fh->hostname, mode));

	if (mode & FMF_WRITE)
	    flags = GENERIC_WRITE;
	if (mode & FMF_READ)
	    flags |= GENERIC_READ;

	/*
	 * FILE_SHARE_WRITE looks strange here, however without it i can't reopen file which
         * is already open with MODE_OLDFILE, even just for reading with FMF_READ
	 */
	lock = (mode & FMF_LOCK) ? 0 : FILE_SHARE_READ|FILE_SHARE_WRITE;

	if (mode & FMF_CREATE)
	    create = (mode & FMF_CLEAR) ? CREATE_ALWAYS : OPEN_ALWAYS;
	else
	    create = (mode & FMF_CLEAR) ? TRUNCATE_EXISTING : OPEN_EXISTING;

	/*
	 * On post-XP systems files with 'system' attribute may be opened for writing
         * only if we specify FILE_ATTRIBUTE_SYSTEM in CreateFile() call. So we keep
         * if from kind value retrieved earlier by GetFileAttributes().
	 */
	protect = prot_a2w(protect) | (kind & FILE_ATTRIBUTE_SYSTEM);

	DOPEN2(bug("[emul] CreateFile: flags 0x%08lX, lock 0x%08lX, create %lu\n", flags, lock, create));
	Forbid();
	fh->fd = emulbase->pdata.KernelIFace->CreateFile(fh->hostname, flags, lock, NULL, create, protect, NULL);

	if ((mode == FMF_MODE_OLDFILE) && (fh->fd == INVALID_HANDLE_VALUE))
	{
            /*
	     * Hack against two problems: 
	     *
	     * Problem 1: dll's in LIBS:Host and AROSBootstrap.exe are locked against writing by
             * Windows while AROS is running. However we may still read them. MODE_OLDFILE
	     * also requests write access with shared lock, this is why it fails on these files.
	     *
	     * Problem 2: FMF_MODE_OLDFILE requests write access, which fails on files with
	     * read-only attribute.
	     *
             * Here we try to work around these problems by attempting to open the file in read-only mode
             * (FMF_READ) when we discover one of them.
	     *
             * I hope this will not affect files really open in AROS because exclusive lock
             * disallows read access also.
	     */
	    err = emulbase->pdata.KernelIFace->GetLastError();

	    DOPEN2(bug("[emul] Windows error: %u\n", err));
	    switch (err)
	    {
	    case ERROR_SHARING_VIOLATION:
	    case ERROR_ACCESS_DENIED:
		fh->fd = emulbase->pdata.KernelIFace->CreateFile(fh->hostname, GENERIC_READ, lock, NULL, OPEN_EXISTING, protect, NULL);
	    }
        }
        err = emulbase->pdata.KernelIFace->GetLastError();
        Permit();

        DOPEN2(bug("[emul] FileHandle = 0x%08lX\n", fh->fd));

	if (fh->fd == INVALID_HANDLE_VALUE)
	    return Errno_w2a(err, mode);
	    
	fh->type = FHD_FILE;
    }

    return 0;
}

/*********************************************************************************************/

LONG DoRead(struct emulbase *emulbase, struct IOFileSys *iofs, BOOL *async)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
    ULONG res;
    ULONG werr;

    if (fh->type & FHD_STDIO)
    {
        /*
	 * FIXME:
	 * 1. This is not thread-safe.
	 * 2. We should return in non-quick mode after queuing the request, and reply it inside the interrupt.
	 */
        struct AsyncReaderControl *reader = emulbase->pdata.ConsoleReader;

	DASYNC(bug("[emul] Reading %lu bytes asynchronously \n", iofs->io_Union.io_READ.io_Length));
	reader->fh = fh->fd;
	reader->addr = iofs->io_Union.io_READ.io_Buffer;
	reader->len = iofs->io_Union.io_READ.io_Length;
	reader->sig = SIGF_DOS;
	reader->task = FindTask(NULL);
	reader->cmd = ASYNC_CMD_READ;
	SetSignal(0, reader->sig);

	Forbid();
	res = emulbase->pdata.KernelIFace->SetEvent(reader->CmdEvent);
	werr = emulbase->pdata.KernelIFace->GetLastError();
	Permit();

	if (res)
	{
	    Wait(reader->sig);

	    DASYNC(bug("[emul] Read %ld bytes, error %lu\n", reader->.actual, reader->error));
	    iofs->io_Union.io_READ.io_Length = reader->actual;
	    werr = reader->error;

	    if (werr)
		res = FALSE;
	    else
	    {
	        char *c, *d;

	        c = iofs->io_Union.io_READ.io_Buffer;
	        d = c;
	        while (*c) {
	            if ((c[0] == '\r') && (c[1] == '\n')) {
	                c++;
	                iofs->io_Union.io_READ.io_Length--;
	            }
	            *d++ = *c++;
	        }
	    }
	}
    }
    else
    {
	Forbid();
	res = emulbase->pdata.KernelIFace->ReadFile(fh->fd, iofs->io_Union.io_READ.io_Buffer,
						      iofs->io_Union.io_READ.io_Length, &iofs->io_Union.io_READ.io_Length, NULL);
	werr = emulbase->pdata.KernelIFace->GetLastError();
	Permit();
    }

    return res ? 0 : Errno_w2a(werr, FMF_MODE_OLDFILE);
}

LONG DoWrite(struct emulbase *emulbase, struct IOFileSys *iofs, BOOL *async)
{
    struct filehandle *fh = (struct filehandle *)iofs->IOFS.io_Unit;
    ULONG error, werr;

    Forbid();
    error = emulbase->pdata.KernelIFace->WriteFile(fh->fd, iofs->io_Union.io_WRITE.io_Buffer,
						   iofs->io_Union.io_WRITE.io_Length, &iofs->io_Union.io_WRITE.io_Length, NULL);
    werr = emulbase->pdata.KernelIFace->GetLastError();
    Permit();

    return error ? 0 : Errno_w2a(werr, FMF_WRITE);
}

static LONG seek_file(struct emulbase *emulbase, void *fh, UQUAD *Offset, ULONG mode, UQUAD *newpos)
{
    ULONG error, werror;
    ULONG pos_high = 0;
    UQUAD oldpos;

    DB2(bug("[emul] LSeek() - getting current position\n"));
    Forbid();
    oldpos = emulbase->pdata.KernelIFace->SetFilePointer(fh, 0, &pos_high, FILE_CURRENT);
    Permit();
    oldpos |= (UQUAD)pos_high << 32;
    DSEEK(bug("[emul] Original position: %llu\n", oldpos));

    switch(mode)
    {
    case OFFSET_BEGINNING:
	mode = FILE_BEGIN;
	break;

    case OFFSET_CURRENT:
	mode = FILE_CURRENT;
	break;

    default:
	mode = FILE_END;
    }

    pos_high = *Offset >> 32;
    DB2(bug("[emul] LSeek() - setting new position\n"));
    Forbid();
    error = emulbase->pdata.KernelIFace->SetFilePointer(fh, *Offset, &pos_high, mode);
    werror = emulbase->pdata.KernelIFace->GetLastError();
    Permit();

    if (error == (ULONG)-1)
	error = Errno_w2a(werror, FMF_MODE_OLDFILE);
    else
    {
	if (newpos)
	{
	    *newpos = error;
	    *newpos |= (UQUAD)pos_high << 32;
	}
	error = 0;
	
	*Offset = oldpos;
    }

    return error;
}


LONG DoSeek(struct emulbase *emulbase, void *fh, UQUAD *Offset, ULONG mode)
{
    return seek_file(emulbase, fh, Offset, mode, NULL);
}

/*********************************************************************************************/

LONG DoMkDir(struct emulbase *emulbase, struct filehandle *fh, ULONG protect)
{
    ULONG ret, werror;

    Forbid();
    ret = emulbase->pdata.KernelIFace->CreateDirectory(fh->hostname, NULL);
    werror = emulbase->pdata.KernelIFace->GetLastError();
    Permit();

    if (ret)
    {
    	fh->type = FHD_DIRECTORY;
	fh->fd   = INVALID_HANDLE_VALUE;
	fh->ph.dirpos = 0;

	Forbid();
	emulbase->pdata.KernelIFace->SetFileAttributes(fh->hostname, prot_a2w(protect));
	Permit();

	return 0;
    }
    return Errno_w2a(werror, FMF_WRITE);
}

/*********************************************************************************************/

LONG DoDelete(struct emulbase *emulbase, char *file)
{
    ULONG ret, err;
  
    Forbid();

    ret = emulbase->pdata.KernelIFace->GetFileAttributes(file);
    if (ret != INVALID_FILE_ATTRIBUTES)
    {
	if (ret & FILE_ATTRIBUTE_DIRECTORY)
            ret = emulbase->pdata.KernelIFace->RemoveDirectory(file);
        else
            ret = emulbase->pdata.KernelIFace->_DeleteFile(file);
    }
    else
	ret = 0;

    err = emulbase->pdata.KernelIFace->GetLastError();

    Permit();

    return ret ? 0 : Errno_w2a(err, 0);
}

/*********************************************************************************************/

LONG DoChMod(struct emulbase *emulbase, char *filename, ULONG aprot)
{
    LONG ret;
    ULONG err;
    
    aprot = prot_a2w(aprot);

    Forbid();
    ret = emulbase->pdata.KernelIFace->SetFileAttributes(filename, aprot);
    err = emulbase->pdata.KernelIFace->GetLastError();
    Permit();

    return ret ? 0 : Errno_w2a(err, FMF_WRITE);
}

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

/*********************************************************************************************/

ULONG examine_start(struct emulbase *emulbase, struct filehandle *fh)
{
    char *c;
    ULONG len;

    if (fh->type != FHD_DIRECTORY)
	return ERROR_OBJECT_WRONG_TYPE;

    if (!fh->ph.pathname)
    {
        DEXAM(bug("[emul] Creating search path for object: %s\n", fh->hostname));
        len = strlen(fh->hostname);
        fh->ph.pathname = AllocVecPooled(emulbase->mempool, len + 3);
        if (!fh->ph.pathname)
            return ERROR_NO_FREE_STORE;

	    CopyMem(fh->hostname, fh->ph.pathname, len);
        c = fh->ph.pathname + len;
        strcpy(c, "\\*");
    }
    DEXAM(bug("[emul] Created search path: %s\n", fh->ph.pathname));
    return 0;
}

/*********************************************************************************************/

/* Reset dirpos in directory handle and close existing search handle */
LONG DoRewindDir(struct emulbase *emulbase, struct filehandle *fh)
{
    ULONG r, err;

    if (fh->fd != INVALID_HANDLE_VALUE)
    {
        Forbid();
        r = emulbase->pdata.KernelIFace->FindClose(fh->fd);
	err = emulbase->pdata.KernelIFace->GetLastError();
        Permit();

        if (!r)
            return Errno_w2a(err, FMF_MODE_OLDFILE);

	fh->fd = INVALID_HANDLE_VALUE;
    }

    fh->ph.dirpos = 0;
    return 0;
}

/*********************************************************************************************/

#define is_special_dir(x) (x[0] == '.' && (!x[1] || (x[1] == '.' && !x[2])))

/* Positions to dirpos in directory, retrieves next item in it and updates dirpos */
static ULONG ReadDir(struct emulbase *emulbase, struct filehandle *fh, LPWIN32_FIND_DATA FindData, IPTR *dirpos)
{
    ULONG res;

    /*
     * Windows does not support positioning within directory. The only thing i can do is to
     * scan the directory in forward direction. In order to bypass this limitation we do the
     * following:
     * 1. Before starting we explicitly set current position (dirpos) to 0. Examine() will place
     *    it into our fib_DiskKey; in case of ExAll() this is eac_LastKey. We also maintain second
     *	directory position counter - in our directory handle. It reflects the real position of
     *	our file search handle.
     * 2. Here we compare position in dirpos with position in the handle. If dirpos is smaller than
     *    filehandle's counter, we have to rewind the directory. This is done by closing the search
     *    handle in order to be able to restart from the beginning and setting handle's counter to 0.
     */
    DEXAM(bug("[emul] Current dirpos %lu, requested %lu\n", fh->ph.dirpos, *dirpos));
    if (fh->ph.dirpos > *dirpos)
    {
	DEXAM(bug("[emul] Resetting search handle\n"));
	DoRewindDir(emulbase, fh);
    }

    do
    {
    /* 
     * 3. Now we will scan the next directory entry until its index is greater than original index
     *    in dirpos. This means that we've repositioned and scanned the next entry. After this we
     *	update dirpos.
     */
	do
	{
	    ULONG err;

            if (fh->fd == INVALID_HANDLE_VALUE)
	    {
		DEXAM(bug("[emul] Finding first file\n"));
		Forbid();
		fh->fd = emulbase->pdata.KernelIFace->FindFirstFile(fh->ph.pathname, FindData);
		err = emulbase->pdata.KernelIFace->GetLastError();
		Permit();
		res = (fh->fd != INVALID_HANDLE_VALUE);
            }
	    else
	    {
		Forbid();
		res = emulbase->pdata.KernelIFace->FindNextFile(fh->fd, FindData);
		err = emulbase->pdata.KernelIFace->GetLastError();
		Permit();
            }
            if (!res)
		return Errno_w2a(err, FMF_MODE_OLDFILE);

		fh->ph.dirpos++;
	    DEXAM(bug("[emul] Found %s, position %lu\n", FindData->cFileName, fh->ph.dirpos));
        } while (fh->ph.dirpos <= *dirpos);
        (*dirpos)++;
        DEXAM(bug("[emul] New dirpos: %lu\n", *dirpos));
    /*
     * We also skip "." and ".." entries (however we count their indexes - just in case), because
     * AmigaOS donesn't have them.
     */
    } while (is_special_dir(FindData->cFileName));

    return 0;
}

/*********************************************************************************************/

static ULONG examine_entry_sub(struct emulbase *emulbase, struct filehandle *fh, STRPTR FoundName, WIN32_FILE_ATTRIBUTE_DATA *FIB)
{
    STRPTR filename, name;
    ULONG plen, flen;
    ULONG error = 0;
    ULONG werr;

    DEXAM(bug("[emul] examine_entry_sub(): filehandle's path: %s\n", fh->hostname));
    if (FoundName)
    {
	DEXAM(bug("[emul] ...containing object: %s\n", FoundName));
	plen = strlen(fh->hostname);
	flen = strlen(FoundName);
	name = AllocVecPooled(emulbase->mempool, plen + flen + 2);
	if (!name)
	    return ERROR_NO_FREE_STORE;

        strcpy(name, fh->hostname);
	filename = name + plen;
	*filename++ = '\\';
	strcpy(filename, FoundName);
    }
    else
	name = fh->hostname;

    DEXAM(bug("[emul] Full name: %s\n", name));
    Forbid();
    error = emulbase->pdata.KernelIFace->GetFileAttributesEx(name, 0, FIB);
    werr = emulbase->pdata.KernelIFace->GetLastError();
    Permit();

    if (FoundName)
    {
	DEXAM(bug("[emul] Freeing full name\n"));
	FreeVecPooled(emulbase->mempool, name);
    }

    return error ? 0 : Errno_w2a(werr, FMF_MODE_OLDFILE);
}	

/*********************************************************************************************/

LONG examine_entry(struct emulbase *emulbase, struct filehandle *fh, char *FoundName,
		    struct ExAllData *ead, ULONG size, ULONG type)
{
    STRPTR next, last, end, name;
    WIN32_FILE_ATTRIBUTE_DATA FIB;
    ULONG error;

    /* Check, if the supplied buffer is large enough. */
    next=(STRPTR)ead+sizes[type];
    end =(STRPTR)ead+size;
  
    if(next > end)
    {
        DEXAM(bug("[emul] examine_entry(): end of buffer\n"));
        return ERROR_BUFFER_OVERFLOW;
    }

    error = examine_entry_sub(emulbase, fh, FoundName, &FIB);
    if (error)
	return error;

    DEXAM(bug("[emul] Filling in object information\n"));
    switch(type)
    {
    default:
    case ED_OWNER:
	ead->ed_OwnerUID = 0;
	ead->ed_OwnerGID = 0;
    case ED_COMMENT:
	ead->ed_Comment = NULL;
	/* TODO: Write Windows shell-compatible comments support using NTFS streams */
    case ED_DATE:
	FileTime2DateStamp((struct DateStamp *)&ead->ed_Days, FIB.ftLastWriteTime);
    case ED_PROTECTION:
	ead->ed_Prot 	= prot_w2a(FIB.dwFileAttributes);
    case ED_SIZE:
	ead->ed_Size	= FIB.nFileSizeLow;
    case ED_TYPE:
	if (FIB.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
	    if (FoundName || fh->name[0])
	        ead->ed_Type = ST_USERDIR;
	    else
		ead->ed_Type = ST_ROOT;
	}
	else
	    ead->ed_Type = ST_FILE;
    case ED_NAME:
	if (FoundName)
	    last = FoundName;
	else if (*fh->name) {
	    name = fh->name;
	    last = name;
	    while(*name)
		if(*name++=='\\')
		    last = name;
	} else
	    last = fh->volumename;
	ead->ed_Name=next;
	for(;;)
	{
	    if (next>=end)
		return ERROR_BUFFER_OVERFLOW;
	    if (!(*next++=*last++))
		break;
	}
    case 0:
	ead->ed_Next=(struct ExAllData *)(((IPTR)next+AROS_PTRALIGN-1)&~(AROS_PTRALIGN-1));
	return 0;
    }
}

/*********************************************************************************************/

LONG examine_next(struct emulbase *emulbase,  struct filehandle *fh, struct FileInfoBlock *FIB)
{
    char *src, *dest;
    ULONG res;
    WIN32_FIND_DATA FindData;
    WIN32_FILE_ATTRIBUTE_DATA AttrData;

    res = examine_start(emulbase, fh);
    if (res)
	return res;

    res = ReadDir(emulbase, fh, &FindData, &FIB->fib_DiskKey);
    if (!res)
	res = examine_entry_sub(emulbase, fh, FindData.cFileName, &AttrData);
    if (res)
    {
	DoRewindDir(emulbase, fh);
	return res;
    }

    FIB->fib_OwnerUID	  = 0;
    FIB->fib_OwnerGID	  = 0;
    FIB->fib_Comment[0]	  = '\0'; /* TODO: no comments available yet! */
    FIB->fib_Protection	  = prot_w2a(AttrData.dwFileAttributes);
    FIB->fib_Size           = AttrData.nFileSizeLow;

    FileTime2DateStamp(&FIB->fib_Date, AttrData.ftLastWriteTime);
    if (AttrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	FIB->fib_DirEntryType = ST_USERDIR;
    else
	FIB->fib_DirEntryType = ST_FILE;

    src  = FindData.cFileName;
    dest = FIB->fib_FileName;
    for (res = 0; res<MAXFILENAMELENGTH-1; res++)
    {
	if (!(*dest++=*src++))
	    break;
    }
    return 0;
}

/*********************************************************************************************/

LONG examine_all(struct emulbase *emulbase, struct filehandle *fh, struct ExAllData *ead,
		 struct ExAllControl *eac, ULONG size, ULONG type)
{
    struct ExAllData *last = NULL;
    STRPTR end = (STRPTR)ead + size;
    LONG error;
    WIN32_FIND_DATA FindData;

    DEXAM(bug("[emul] examine_all(%s)\n", fh->hostname));

    eac->eac_Entries = 0;
    error = examine_start(emulbase, fh);
    if (error)
	return error;

    for(;;)
    {
	error = ReadDir(emulbase, fh, &FindData, &eac->eac_LastKey);
	if (error) {
            DEXAM(bug("[emul] ReadDir() returned %lu\n", error));
            break;
        }

	/* Try to match the filename, if required.  */
	DEXAM(bug("[emul] Checking against MatchString\n"));
	if (eac->eac_MatchString && !MatchPatternNoCase(eac->eac_MatchString, FindData.cFileName))
	    continue;

	DEXAM(bug("[emul] Examining object\n"));
        error = examine_entry(emulbase, fh, FindData.cFileName, ead, end-(STRPTR)ead, type);
        if(error)
	    break;
	/* Do some more matching... */
	if ((eac->eac_MatchFunc) && !CALLHOOKPKT(eac->eac_MatchFunc, ead, &type))
	    continue;
	eac->eac_Entries++;
	last = ead;
	ead = ead->ed_Next;
    }

    if (last!=NULL)
	last->ed_Next=NULL;

    if ((error==ERROR_BUFFER_OVERFLOW) && last)
    {
	/*
	 * ERROR_BUFFER_OVERFLOW happened while examining the last entry.
	 * We need to step back to it in order to re-examine it next time.
	 */
	eac->eac_LastKey--;
	return 0;
    }

    return error;
}

/*********************************************************************************************/

LONG DoHardLink(struct emulbase *emulbase, char *name, char *oldfile)
{
    ULONG error, werr;

    if (!emulbase->pdata.KernelIFace->CreateHardLink)
	return ERROR_ACTION_NOT_KNOWN;

    DLINK(bug("[emul] Creating hardlink %s to file %s\n", name, oldfile));
    Forbid();
    error = emulbase->pdata.KernelIFace->CreateHardLink(name, oldfile, NULL);
    werr = emulbase->pdata.KernelIFace->GetLastError();
    Permit();

    return error ? 0 : Errno_w2a(werr, FMF_WRITE);
}

/*********************************************************************************************/

LONG DoSymLink(struct emulbase *emulbase, char *dest, char *src)
{
    ULONG error, werr;

/* This subroutine is intentionally disabled because:
   1. On Windows 7 CreateSymbolicLink() gives ERROR_PRIVILEGE_NOT_HELD error
   2. Referred object name is supplied in AROS form. Windows seems not to like it,
      and it needs to be translated to Windows path. This will not work in all cases
      and this requires additional thinking and coding. Since reading symbolic links
      is not implemented yet, i disabled creation too - Pavel Fedin <pavel_fedin@mail.ru>
    if (!emulbase->pdata.KernelIFace->CreateSymbolicLink) */
	return ERROR_ACTION_NOT_KNOWN;

    Forbid();
    error = emulbase->pdata.KernelIFace->CreateSymbolicLink(dest, src, 0);
    werr = emulbase->pdata.KernelIFace->GetLastError();
    Permit();

    DLINK(bug("[emul] Result: %d, Windows error: %u\n", error, werr));

    return error ? 0 : Errno_w2a(werr, FMF_WRITE);
}

/*********************************************************************************************/

LONG DoRename(struct emulbase *emulbase, char *filename, char *newname)
{
    ULONG ret, werr;

    Forbid();
    ret = emulbase->pdata.KernelIFace->MoveFile(filename, newname);
    werr = emulbase->pdata.KernelIFace->GetLastError();
    Permit();

    return ret ? 0 : Errno_w2a(werr, FMF_WRITE);
}

/*********************************************************************************************/

int DoReadLink(struct emulbase *emulbase, char *name, char *buffer, ULONG size, LONG *errPtr)
{
    /* TODO: implement this. */
    *errPtr = ERROR_ACTION_NOT_KNOWN;
    return -1;
}

/*********************************************************************************************/

LONG DoSetDate(struct emulbase *emulbase, char *fullname, struct DateStamp *date)
{
    void *handle;
    LONG ret;
    ULONG werr;
    UQUAD ft;

    ft = DateStamp2FileTime(date);

    Forbid();
    handle = emulbase->pdata.KernelIFace->CreateFile(fullname, FILE_WRITE_ATTRIBUTES,
						     FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
						     NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE)
        ret = 0;
    else
	ret = emulbase->pdata.KernelIFace->SetFileTime(handle, &ft, NULL, &ft);

    werr = emulbase->pdata.KernelIFace->GetLastError();

    if (handle != INVALID_HANDLE_VALUE)
	emulbase->pdata.KernelIFace->CloseHandle(handle);

    Permit();

    return ret ? 0 : Errno_w2a(werr, FMF_WRITE);
}

LONG DoSetSize(struct emulbase *emulbase, struct filehandle *fh, struct IFS_SEEK *io_SEEK)
{
    LONG error;
    ULONG werr;
    UQUAD newpos;

    DFSIZE(bug("[emul] DoSetSize(): mode %ld, offset %llu\n", io_SEEK->io_SeekMode, io_SEEK->io_Offset));

    /* First seek to the requested position. io_Offset will contain OLD position after that. NEW position will be in newpos */
    error = seek_file(emulbase, fh, &io_SEEK->io_Offset, io_SEEK->io_SeekMode, &newpos);
    if (!error)
    {
        /* Set EOF to NEW position */
        Forbid();
        error = emulbase->pdata.KernelIFace->SetEndOfFile(fh->fd);
	werr = emulbase->pdata.KernelIFace->GetLastError();
        Permit();

	error = error ? 0 : Errno_w2a(werr, FMF_WRITE);

	/*
	 * If our OLD position was less than new file size, we seek back to it. io_Offset will again contain
         * position before this seek - i. e. our NEW file size.
	 */
        if (io_SEEK->io_Offset < newpos)
	{
	    LONG error2;

            error2 = seek_file(emulbase, fh, &io_SEEK->io_Offset, OFFSET_BEGINNING, NULL);
            if (!error)
        	error = error2;
        } else
            io_SEEK->io_Offset = newpos;
    }

    DFSIZE(bug("[emul] FSA_SET_FILE_SIZE returning %lu\n", error));
    return error;
}

BOOL DoGetType(struct emulbase *emulbase, void *fd)
{
    ULONG type;

    DB2(bug("[emul] GetFileType()\n"));

    Forbid();
    type = emulbase->pdata.KernelIFace->GetFileType(fd);
    Permit();

    return type == FILE_TYPE_CHAR ? TRUE : FALSE;
}

LONG DoStatFS(struct emulbase *emulbase, char *path, struct InfoData *id)
{
    char *c;
    char s;
    ULONG SectorsPerCluster, BytesPerSector, FreeBlocks;
    ULONG res, err;

    DSTATFS(printf("[EmulHandler] StatFS(\"%s\")\n", path));

    /* GetDiskFreeSpace() can be called only on root path. We always work with absolute pathnames, so let's get first part of the path */
    c = path;
    if ((c[0] == '\\') && (c[1] == '\\'))
    {
	/* If the path starts with "\\", it's a UNC path. Its root is "\\Server\Share\", so we skip "\\Server\" part */
	c += 2;
	while (*c != '\\') {
            if (*c == 0)
		return ERROR_OBJECT_NOT_FOUND;
            c++;
	}
	c++;
    }

    /* Skip everything up to the first '\'. */
    while (*c != '\\')
    {
	if (*c == 0)
	    return ERROR_OBJECT_NOT_FOUND;
	c++;
    }

    Forbid();

    /* Temporarily terminate the path */
    s = c[1];
    c[1] = 0;
    DSTATFS(printf("[EmulHandler] Root path: %s\n", path));

    res = emulbase->pdata.KernelIFace->GetDiskFreeSpace(path, &SectorsPerCluster, &BytesPerSector, &FreeBlocks, &id->id_NumBlocks);
    err = emulbase->pdata.KernelIFace->GetLastError();

    c[1] = s;
    Permit();

    if (res)
    {
	id->id_NumSoftErrors = 0;
	id->id_UnitNumber = 0;
	id->id_DiskState = ID_VALIDATED;
	id->id_NumBlocksUsed = id->id_NumBlocks - FreeBlocks;
	id->id_BytesPerBlock = SectorsPerCluster*BytesPerSector;

	return 0;
    }

    return Errno_w2a(err, FMF_MODE_OLDFILE);
}

char *GetHomeDir(struct emulbase *emulbase, char *sp)
{
    char home[260];
    char *newunixpath = NULL;
    char *sp_end;
    WORD cmplen;
    char tmp;
    ULONG err;

    /* "~<name>" means home of user <name> */		
    for(sp_end = sp; sp_end[0] != '\0' && sp_end[0] != '\\'; sp_end++);

    cmplen = sp_end - sp - 1;
    /* temporarily zero terminate name */
    tmp = sp[cmplen+1];
    sp[cmplen+1] = '\0';

    Forbid();
    err = emulbase->pdata.EmulIFace->EmulGetHome(sp+1, home);
    Permit();

    sp[cmplen+1] = tmp;

    if (!err)
    {
    	int hlen = strlen(home);
	int splen = strlen(sp_end);

	newunixpath = AllocVec(hlen + splen + 1, MEMF_PUBLIC);
	if (newunixpath)
	{
	    char *s = newunixpath;

	    CopyMem(home, s, hlen);
	    s += hlen;
	    CopyMem(sp_end, s, splen);
	    s += splen;
	    *s = 0;
	}
    }

    return newunixpath;
}

ULONG GetCurrentDir(struct emulbase *emulbase, char *path, ULONG len)
{
    ULONG res;

    Forbid();
    res = emulbase->pdata.KernelIFace->GetCurrentDirectory(len, path);
    Permit();

    return (res < len) ? TRUE : FALSE;
}

int CheckDir(struct emulbase *emulbase, char *path)
{
    ULONG attrs;

    Forbid();
    attrs = emulbase->pdata.KernelIFace->GetFileAttributes(path);
    Permit();

    if (attrs == INVALID_FILE_ATTRIBUTES)
	return -1;

    return (attrs & FILE_ATTRIBUTE_DIRECTORY) ? 0 : -1;
}

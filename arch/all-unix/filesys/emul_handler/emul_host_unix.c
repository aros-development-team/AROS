/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "unix_hints.h"

#ifdef HOST_LONG_ALIGNED
#pragma pack(4)
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#pragma pack()

/* This prevents redefinition of struct timeval */
#define _AROS_TYPES_TIMEVAL_S_H_

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <dos/dosasl.h>
#include <utility/date.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "emul_intern.h"
#include "emul_unix.h"

#define NO_CASE_SENSITIVITY

#ifdef DEBUG_INTERFACE
#define DUMP_INTERFACE					\
{							\
    int i;						\
    APTR *iface = (APTR *)emulbase->pdata.SysIFace;	\
							\
    for (i = 0; libcSymbols[i]; i++)			\
	bug("%s\t\t0x%p\n", libcSymbols[i], iface[i]);	\
}
#else
#define DUMP_INTERFACE
#endif

static const char *libcSymbols[] =
{
    "open" UNIX2003_SUFFIX,
    "close" UNIX2003_SUFFIX,
    "closedir" UNIX2003_SUFFIX,
    "opendir" UNIX2003_SUFFIX,
    "readdir" INODE64_SUFFIX,
    "rewinddir" UNIX2003_SUFFIX,
    "read" UNIX2003_SUFFIX,
    "write" UNIX2003_SUFFIX,
    "lseek",
    "ftruncate",
    "mkdir",
    "rmdir",
    "unlink",
    "link",
    "symlink",
    "readlink",
    "rename",
    "chmod" UNIX2003_SUFFIX,
    "isatty",
    "statfs",
    "utime",
    "localtime",
    "mktime" UNIX2003_SUFFIX,
    "getcwd",
    "getenv",
    "poll" UNIX2003_SUFFIX,
#ifdef HOST_OS_linux
    "__xstat",
    "__lxstat",
#else
    "stat" INODE64_SUFFIX,
    "lstat" INODE64_SUFFIX,
#endif
#ifndef HOST_OS_android
    "seekdir" UNIX2003_SUFFIX,
    "telldir" UNIX2003_SUFFIX,
    "getpwent",
    "endpwent",
#endif
    NULL
};

/*********************************************************************************************/

static inline struct filehandle *CreateStdHandle(int fd)
{
    struct filehandle *fh;

    fh = AllocMem(sizeof(struct filehandle), MEMF_PUBLIC|MEMF_CLEAR);
    if (fh)
    {
	fh->type = FHD_FILE|FHD_STDIO;
	fh->fd   = (void *)(IPTR)fd;
    }

    return fh;
}

static int host_startup(struct emulbase *emulbase)
{
    ULONG r = 0;

    OOPBase = OpenLibrary("oop.library", 0);
    if (!OOPBase)
    	return FALSE;

    UtilityBase = OpenLibrary("utility.library", 0);
    D(bug("[EmulHandler] UtilityBase = %p\n", UtilityBase));
    if (!UtilityBase)
	return FALSE;

    emulbase->pdata.em_UnixIOBase = (struct UnixIOBase *)OpenLibrary("unixio.hidd", 43);
    if (!emulbase->pdata.em_UnixIOBase)
    	return FALSE;

    emulbase->pdata.unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!emulbase->pdata.unixio)
    	return FALSE;

    emulbase->pdata.SysIFace = (struct LibCInterface *)HostLib_GetInterface(emulbase->pdata.em_UnixIOBase->uio_LibcHandle, libcSymbols, &r);
    if (!emulbase->pdata.SysIFace)
    {
        D(bug("[EmulHandler] Unable to get host-side library interface!\n"));
    	CloseLibrary(UtilityBase);
    	return FALSE;
    }

    D(bug("[EmulHandler] %lu unresolved symbols\n", r));
    DUMP_INTERFACE
    if (r)
    {
    	CloseLibrary(UtilityBase);
    	return FALSE;
    }

    /* Cache errno pointer for faster access */
    emulbase->pdata.errnoPtr = emulbase->pdata.em_UnixIOBase->uio_ErrnoPtr;

    /* Create handles for emergency console */
    emulbase->eb_stdin  = CreateStdHandle(STDIN_FILENO);
    emulbase->eb_stdout = CreateStdHandle(STDOUT_FILENO);
    emulbase->eb_stderr = CreateStdHandle(STDERR_FILENO);

    return TRUE;
}

ADD2INITLIB(host_startup, 0);

static int host_cleanup(struct emulbase *emulbase)
{
    D(bug("[EmulHandler] Expunge\n"));

    if (emulbase->pdata.SysIFace)
    	HostLib_DropInterface((APTR *)emulbase->pdata.SysIFace);

    /* UnixIO v42 object is a singletone, we don't need to dispose it */

    CloseLibrary(OOPBase);
    CloseLibrary(&emulbase->pdata.em_UnixIOBase->uio_Library);
    CloseLibrary(UtilityBase);

    return TRUE;
}

ADD2EXPUNGELIB(host_cleanup, 0);
 

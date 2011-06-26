/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef HOST_OS_ios

#ifdef __arm__
/*
 * Under ARM iOS quadwords are long-aligned, however in AROS (according to AAPCS)
 * they are quad-aligned. This macro turns on some tricks which bypass this problem
 */
#define HOST_LONG_ALIGNED
#endif
#ifdef __i386__
/*
 * Under i386 we pick up MacOS' libSystem.dylib instead of Simulator's libSystem.dylib,
 * so we have to use special versions of certain functions. We can't simply #define _DARWIN_NO_64_BIT_INODE
 * because iOS SDK forbids this (in iOS inode_t is always 64-bit wide)
 */
#define INODE64_SUFFIX "$INODE64"
#endif

#else

/* 
 * Use 32-bit inode_t on Darwin. Otherwise we are expected to use "stat$INODE64"
 * instead of "stat" function which is available only on MacOS 10.6.
 */
#define _DARWIN_NO_64_BIT_INODE
#endif

#ifndef INODE64_SUFFIX
#define INODE64_SUFFIX
#endif

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

#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <dos/dosasl.h>
#include <utility/date.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "emul_intern.h"

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

static const char *libcSymbols[] = {
    "open",
    "close",
    "closedir",
    "opendir",
    "readdir" INODE64_SUFFIX,
    "rewinddir",
    "read",
    "write",
    "lseek",
    "ftruncate",
    "mkdir",
    "rmdir",
    "unlink",
    "link",
    "symlink",
    "readlink",
    "rename",
    "chmod",
    "isatty",
    "statfs",
    "utime",
    "localtime",
    "mktime",
    "getcwd",
    "getenv",
    "fcntl",
    "select",
    "kill",
    "getpid",
#ifndef HOST_OS_android
    "seekdir",
    "telldir",
    "getpwent",
    "endpwent",
#endif
#ifdef HOST_OS_linux
    "__errno_location",
    "__xstat",
    "__lxstat",
#else
#ifdef HOST_OS_android
    "__errno",
#else
    "__error",
#endif
    "stat" INODE64_SUFFIX,
    "lstat" INODE64_SUFFIX,
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

    HostLibBase = OpenResource("hostlib.resource");

    D(bug("[EmulHandler] got hostlib.resource %p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    KernelBase = OpenResource("kernel.resource");
    D(bug("[EmulHandler] KernelBase = %p\n", KernelBase));
    if (!KernelBase)
	return FALSE;

    UtilityBase = OpenLibrary("utility.library", 0);
    D(bug("[EmulHandler] UtilityBase = %p\n", UtilityBase));
    if (!UtilityBase)
	return FALSE;

    emulbase->pdata.libcHandle = HostLib_Open(LIBC_NAME, NULL);
    D(bug("[EmulHandler] HostLib = %p\n", emulbase->pdata.libcHandle));
    if (!emulbase->pdata.libcHandle) {
    	CloseLibrary(UtilityBase);
    	return FALSE;
    }

    emulbase->pdata.SysIFace = (struct LibCInterface *)HostLib_GetInterface(emulbase->pdata.libcHandle, libcSymbols, &r);
    if (!emulbase->pdata.SysIFace)
    {
        D(bug("[EmulHandler] Unable go get host-side library interface!\n"));
    	CloseLibrary(UtilityBase);
    	return FALSE;
    }

    D(bug("[EmulHandler] %lu unresolved symbols\n", r));
    DUMP_INTERFACE
    if (r) {
    	CloseLibrary(UtilityBase);
    	return FALSE;
    }

    emulbase->eb_stdin  = CreateStdHandle(STDIN_FILENO);
    emulbase->eb_stdout = CreateStdHandle(STDOUT_FILENO);
    emulbase->eb_stderr = CreateStdHandle(STDERR_FILENO);

    NEWLIST(&emulbase->pdata.readList);
    emulbase->pdata.my_pid   = emulbase->pdata.SysIFace->getpid();
    AROS_HOST_BARRIER
    emulbase->pdata.errnoPtr = emulbase->pdata.SysIFace->__error();
    AROS_HOST_BARRIER

    return TRUE;
}

ADD2INITLIB(host_startup, 0);

static int host_cleanup(struct emulbase *emulbase)
{
    D(bug("[EmulHandler] Expunge\n"));

    if (emulbase->pdata.SysIFace)
    	HostLib_DropInterface((APTR *)emulbase->pdata.SysIFace);

    if (emulbase->pdata.libcHandle)
    	HostLib_Close(emulbase->pdata.libcHandle, NULL);

    CloseLibrary(UtilityBase);

    return TRUE;
}

ADD2EXPUNGELIB(host_cleanup, 0);
 

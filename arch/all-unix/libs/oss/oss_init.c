/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/intuition.h>
#include <proto/kernel.h>

#include <aros/symbolsets.h>

#include <string.h>

#include LC_LIBDEFS_FILE

int audio_fd;
extern struct OSS_Base *GlobalOSSBase;

static BOOL CheckArch(STRPTR MyArch)
{
    APTR KernelBase;
    STRPTR archs[2] = {MyArch, NULL};

    KernelBase = OpenResource("kernel.resource");
    D(bug("[OSS] KernelBase0x%p\n", KernelBase));
    if (!KernelBase)
	return FALSE;

    archs[1] = (STRPTR)KrnGetSystemAttr(KATTR_Architecture);
    D(bug("[OSS] My architecture: %s, kernel architecture: %s\n", archs[0], archs[1]));
    if (!archs[1])
	return FALSE;

    if (strcmp(archs[0], archs[1]))
    {
	struct IntuitionBase *IntuitionBase;

	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
	if (IntuitionBase)
	{
            struct EasyStruct es = {
        	sizeof (struct EasyStruct),
        	0,
        	"Incompatible architecture",
		"This version of oss.library is built for use\n"
		"with %s architecture, but your\n"
		"system architecture is %s.",
        	"Ok",
	    };

	    EasyRequestArgs(NULL, &es, NULL, (IPTR *)archs);

	    CloseLibrary(&IntuitionBase->LibNode);
	}
	return FALSE;
    }

    D(bug("[OSS] Architecture check done\n"));
    return TRUE;
}

static const char *libc_symbols[] = {
    "open",
    "close",
    "ioctl",
    "mmap",
    "munmap",
    "write",
#ifdef HOST_OS_linux
    "__errno_location",
#else
    "__error",
#endif
    NULL
};

static int InitData(LIBBASETYPEPTR OSSBase)
{
    ULONG r = 0;

    D(bug("[OSS] Init\n"));

    if (!CheckArch(AROS_ARCHITECTURE))
	return FALSE;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[OSS] HostLibBase is %p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    OSSBase->LibCHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!OSSBase->LibCHandle)
	return FALSE;

    OSSBase->OSSIFace = (struct LibCInterface *)HostLib_GetInterface(OSSBase->LibCHandle, libc_symbols, &r);
    D(bug("[OSS] libc interface 0x%p, %u unresolved symbols\n", OSSBase->OSSIFace, r));
    if ((!OSSBase->OSSIFace) || r)
	return FALSE;

    InitSemaphore(&OSSBase->sem);
    OSSBase->errnoPtr = OSSBase->OSSIFace->__error();

    GlobalOSSBase = OSSBase;
    return TRUE;
}

static int OpenLib(LIBBASETYPEPTR LIBBASE)
{
    /* Allow only one opener */

    return ((struct Library *)LIBBASE)->lib_OpenCnt ? FALSE : TRUE;
}

static int CleanUp(LIBBASETYPEPTR OSSBase)
{
    if (!HostLibBase)
	return TRUE;

    if (OSSBase->OSSIFace)
	HostLib_DropInterface((APTR *)OSSBase->OSSIFace);

    if (OSSBase->LibCHandle)
	HostLib_Close(OSSBase->LibCHandle, NULL);

    return TRUE;
}

ADD2INITLIB(InitData, 0);
ADD2OPENLIB(OpenLib, 0);
ADD2EXPUNGELIB(CleanUp, 0);

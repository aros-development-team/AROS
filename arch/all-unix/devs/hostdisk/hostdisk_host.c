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

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <devices/trackdisk.h>
#include <proto/exec.h>
#include <proto/hostlib.h>

#ifdef HOST_LONG_ALIGNED
#pragma pack(4)
#endif

#include <errno.h>
#include <fcntl.h>

#pragma pack()

#include "hostdisk_host.h"
#include "hostdisk_device.h"

static ULONG error(int unixerr)
{
    switch (unixerr)
    {
	case EPERM:
	    return TDERR_WriteProt;

	default:
	    return TDERR_NotSpecified;
    }
}

ULONG Host_Open(struct unit *Unit)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;
    struct stat st;
    int err;

    HostLib_Lock();

    Unit->file = hdskBase->iface->open(Unit->filename, O_RDWR, 0755, &err);
    AROS_HOST_BARRIER
    err = *hdskBase->errnoPtr;

    HostLib_Unlock();

    if (Unit->file == -1)
	return error(err);

    Unit->flags = 0;

    HostLib_Lock();
    err = hdskBase->iface->fstat(Unit->file, &st);
    HostLib_Unlock();

    if (err != -1)
    {
        if (S_ISBLK(st.st_mode))
            Unit->flags |= UNIT_DEVICE;
    }

    D(bug("hostdisk: Unit flags 0x%02X\n", Unit->flags));
    return 0;
}

void Host_Close(struct unit *Unit)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;

    HostLib_Lock();

    Unit->hdskBase->iface->close(Unit->file);
    AROS_HOST_BARRIER

    HostLib_Unlock();
}

LONG Host_Read(struct unit *Unit, APTR buf, ULONG size, ULONG *ioerr)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;
    int ret, err;
    
    HostLib_Lock();

    ret = hdskBase->iface->read(Unit->file, buf, size);
    AROS_HOST_BARRIER
    err = *hdskBase->errnoPtr;

    HostLib_Unlock();

    if (ret == -1)
    	*ioerr = error(err);

    return ret;
}

LONG Host_Write(struct unit *Unit, APTR buf, ULONG size, ULONG *ioerr)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;
    int ret, err;

    HostLib_Lock();

    ret = hdskBase->iface->write(Unit->file, buf, size);
    AROS_HOST_BARRIER
    err = *hdskBase->errnoPtr;

    HostLib_Unlock();

    if (ret == -1)
	*ioerr = error(err);

    return ret;
}

ULONG Host_Seek(struct unit *Unit, ULONG pos)
{
    return Host_Seek64(Unit, pos, 0);
}

ULONG Host_Seek64(struct unit *Unit, ULONG pos, ULONG pos_hi)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;
    int res;

    /*
     * Host OS is usually not reentrant.
     * All host OS calls should be protected by global lock (since hostlib.resource v3).
     */
    HostLib_Lock();

    res = LSeek(Unit->file, pos, 0, SEEK_SET);
    AROS_HOST_BARRIER

    HostLib_Unlock();

    return (res == -1) ? TDERR_SeekError : 0;
}

ULONG Host_GetGeometry(struct unit *Unit, struct DriveGeometry *dg)
{
    return TDERR_NotSpecified;
}

extern const char Hostdisk_LibName[];

static const char *libcSymbols[] =
{
    "open",
    "close",
    "read",
    "write",
    "lseek",
#ifdef HOST_OS_linux
    "__errno_location",
    "__fxstat",
#else
#ifdef HOST_OS_android
    "__errno",
#else
    "__error",
#endif
    "fstat" INODE64_SUFFIX,
#endif
    NULL
};

static int Host_Init(struct HostDiskBase *hdskBase)
{
    ULONG r;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[hostdisk] HostLibBase: 0x%p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    hdskBase->KernelHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!hdskBase->KernelHandle)
	return FALSE;

    hdskBase->iface = (struct HostInterface *)HostLib_GetInterface(hdskBase->KernelHandle, libcSymbols, &r);
    if ((!hdskBase->iface) || r)
	return FALSE;

    hdskBase->errnoPtr = hdskBase->iface->__error();
    hdskBase->DiskDevice = "/dev/disk%ld";

    return TRUE;
}

static int Host_Cleanup(struct HostDiskBase *hdskBase)
{
    if (!HostLibBase)
	return TRUE;

    if (hdskBase->iface)
	HostLib_DropInterface((APTR *)hdskBase->iface);

    if (hdskBase->KernelHandle)
	HostLib_Close(hdskBase->KernelHandle, NULL);

    return TRUE;
}

ADD2INITLIB(Host_Init, 0);
ADD2EXPUNGELIB(Host_Cleanup, 0);

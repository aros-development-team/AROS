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

/* Correspondingly, there's no struct stat64. Use struct stat instead. */
#define stat64 stat
#else

/* 
 * Use 32-bit inode_t on Darwin. Otherwise we are expected to use "stat$INODE64"
 * instead of "stat" function which is available only on MacOS 10.6.
 */
#define _DARWIN_NO_64_BIT_INODE
/* This enables struct stat64 definition */
#define _DARWIN_C_SOURCE        /* For Darwin */
#define _LARGEFILE64_SOURCE     /* For Linux */
#endif

#ifndef INODE64_SUFFIX
#define INODE64_SUFFIX
#endif

/* Prevents struct timeval redefinition */
#define timeval sys_timeval

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __linux__
#define stat64 stat
#endif

#undef timeval

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <devices/trackdisk.h>
#include <exec/errors.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/intuition.h>
#include <proto/kernel.h>

#ifdef HOST_LONG_ALIGNED
#pragma pack(4)
#endif

#pragma pack()

#include "hostdisk_host.h"
#include "hostdisk_device.h"

static ULONG error(int unixerr)
{
    D(bug("hostdisk: UNIX error %d\n", unixerr));

    switch (unixerr)
    {
    case 0:
        return 0;

    case EBUSY:
        return TDERR_DriveInUse;

    case EPERM:
        return TDERR_WriteProt;

    default:
        return TDERR_NotSpecified;
    }
}

ULONG Host_Open(struct unit *Unit)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;
    int err;

    D(bug("hostdisk: Host_Open(%s)\n", Unit->filename));
    Unit->flags = 0;

    HostLib_Lock();

    Unit->file = hdskBase->iface->open(Unit->filename, O_RDWR, 0755, &err);
    AROS_HOST_BARRIER
    err = *hdskBase->errnoPtr;

    if (err == EBUSY || err == EROFS)
    {
        /* This allows to work on Darwin, at least in read-only mode */
        D(bug("hostdisk: EBUSY, retrying with read-only access\n", Unit->filename, Unit->file, err));
        Unit->flags = UNIT_READONLY;

        Unit->file = hdskBase->iface->open(Unit->filename, O_RDONLY, 0755);
        AROS_HOST_BARRIER
        err = *hdskBase->errnoPtr;
    }

    HostLib_Unlock();

    if (Unit->file == -1)
    {
        D(bug("hostdisk: Error %d\n", err));

        return error(err);
    }

    return 0;
}

void Host_Close(struct unit *Unit)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;

    D(bug("hostdisk: Close device %s\n", Unit->n.ln_Name));
    D(bug("hostdisk: HostLibBase 0x%p, close() 0x%p\n", HostLibBase, hdskBase->iface->close));

    HostLib_Lock();

    hdskBase->iface->close(Unit->file);
    AROS_HOST_BARRIER

    HostLib_Unlock();
}

LONG Host_Read(struct unit *Unit, APTR buf, ULONG size, ULONG *ioerr)
{
    struct HostDiskBase *hdskBase = Unit->hdskBase;
    int ret, err;

    D(bug("hostdisk: Read %u bytes\n", size));

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

    D(bug("hostdisk: Write %u bytes\n", size));

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

    D(bug("hostdisk: Seek to block 0x%08X%08X (%llu)\n", pos_hi, pos, (((UQUAD)pos_hi) << 32) + pos));

    /*
     * Host OS is usually not reentrant.
     * All host OS calls should be protected by global lock (since hostlib.resource v3).
     */
    HostLib_Lock();

    res = LSeek(Unit->file, pos, pos_hi, SEEK_SET);
    AROS_HOST_BARRIER

    HostLib_Unlock();

    return (res == -1) ? TDERR_SeekError : 0;
}

static ULONG InternalGetGeometry(int file, struct DriveGeometry *dg, struct HostDiskBase *hdskBase)
{
    int res, err;
    struct stat64 st;

    HostLib_Lock();

    res = hdskBase->iface->fstat64(file, &st);
    err = *hdskBase->errnoPtr;

    HostLib_Unlock();

    if (res != -1)
    {
        if (S_ISBLK(st.st_mode))
        {
            /*
             * For real block devices we can use IOCTL-based function. It will provide better info.
             * This routine returns UNIX error code, for simplicity.
             */
            err = Host_DeviceGeometry(file, dg, hdskBase);

           /* If this routine is not implemented, use fstat() (worst case) */
           if (err != ENOSYS)
               return error(err);
        }

        D(bug("hostdisk: Image file length: %ld\n", st.st_size));

        dg->dg_TotalSectors = st.st_size / dg->dg_SectorSize;
        dg->dg_Cylinders    = dg->dg_TotalSectors; /* LBA, CylSectors == 1 */

        return 0;
    }

    D(bug("hostdisk: InternalGetGeometry(): UNIX error %u\n", err));
    return err;
}

ULONG Host_GetGeometry(struct unit *Unit, struct DriveGeometry *dg)
{
    int err = InternalGetGeometry(Unit->file, dg, Unit->hdskBase);
    
    return error(err);
}

int Host_ProbeGeometry(struct HostDiskBase *hdskBase, char *name, struct DriveGeometry *dg)
{
    int file, res;

    HostLib_Lock();

    file = hdskBase->iface->open(name, O_RDONLY, 0755);
    AROS_HOST_BARRIER

    HostLib_Unlock();

    if (file == -1)
        return -1;

    res = InternalGetGeometry(file, dg, hdskBase);

    HostLib_Lock();
    
    hdskBase->iface->close(file);
    AROS_HOST_BARRIER

    HostLib_Unlock();
    
    return res;
}    

extern const char Hostdisk_LibName[];

static const char *libcSymbols[] =
{
    "open",
    "close",
    "read",
    "write",
    "ioctl",
    "lseek",
#ifdef HOST_OS_linux
    "__errno_location",
    "__fxstat64",
    "__xstat64",
#else
#ifdef HOST_OS_android
    "__errno",
#else
    "__error",
#endif
    "fstat64" INODE64_SUFFIX,
    "stat64" INODE64_SUFFIX,
#endif
    NULL
};

static BOOL CheckArch(const char *Component, const char *MyArch, const char *SystemArch)
{
    const char *arg[3] = {Component, MyArch, SystemArch};

    D(bug("hostdisk: My architecture: %s, kernel architecture: %s\n", arg[1], arg[2]));

    if (strcmp(arg[1], arg[2]))
    {
        struct IntuitionBase *IntuitionBase;

        IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36);
        if (IntuitionBase)
        {
            struct EasyStruct es =
            {
                sizeof (struct EasyStruct),
                0,
                "Incompatible architecture",
                "Used version of %s is built for use\n"
                "with %s architecture, but your\n"
                "system architecture is %s.",
                "Ok",
            };

            EasyRequestArgs(NULL, &es, NULL, (IPTR *)arg);

            CloseLibrary(&IntuitionBase->LibNode);
        }
        return FALSE;
    }

    D(bug("hostdisk: Architecture check done\n"));
    return TRUE;
}

static int Host_Init(struct HostDiskBase *hdskBase)
{
    ULONG r;
    STRPTR arch;
    /*
     * This device is disk-based and it can travel from disk to disk.
     * In order to prevent unexplained crashes we check that system architecture
     * is the architecture we were built for.
     */
    APTR KernelBase = OpenResource("kernel.resource");

    if (!KernelBase)
        return FALSE;

    arch = (STRPTR)KrnGetSystemAttr(KATTR_Architecture);
    if (!arch)
        return FALSE;

    if (!CheckArch(Hostdisk_LibName, AROS_ARCHITECTURE, arch))
        return FALSE;

    hdskBase->KernelHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!hdskBase->KernelHandle)
        return FALSE;

    hdskBase->iface = (struct HostInterface *)HostLib_GetInterface(hdskBase->KernelHandle, libcSymbols, &r);
    if ((!hdskBase->iface) || r)
        return FALSE;

    hdskBase->errnoPtr = hdskBase->iface->__error();

    hdskBase->DiskDevice = DISK_DEVICE;
    hdskBase->unitBase   = DISK_BASE;

    return TRUE;
}

ADD2INITLIB(Host_Init, 0);


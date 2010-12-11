#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <devices/trackdisk.h>
#include <proto/exec.h>
#include <proto/hostlib.h>

#include "hostdisk_host.h"
#include "hostdisk_device.h"

static ULONG error(ULONG winerr)
{
    switch(winerr)
    {
/*	case ERROR_SEEK_ERROR:
	    return TDERR_SeekError;*/
	    
	case ERROR_WRITE_PROTECT:
	    return TDERR_WriteProt;

/*	case ERROR_NO_DISK:
	    return TDERR_DiskChanged;*/

	default:
	    return TDERR_NotSpecified;
    }
}

ULONG Host_Open(struct unit *Unit)
{
    ULONG attrs;

    Forbid();
    attrs = Unit->hdskBase->iface->GetFileAttributes(Unit->filename);
    Unit->file = Unit->hdskBase->iface->CreateFile(Unit->filename, GENERIC_READ, FILE_SHARE_VALID_FLAGS, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    Permit();

    if (Unit->file == (APTR)-1)
	return TDERR_NotSpecified;

    Unit->writable = !(attrs & FILE_ATTRIBUTE_READONLY);

    return 0;
}

void Host_Close(struct unit *Unit)
{
    Forbid();
    Unit->hdskBase->iface->CloseHandle(Unit->file);
    Permit();
}

LONG Host_Read(struct unit *Unit, APTR buf, ULONG size, ULONG *ioerr)
{
    ULONG resSize;
    ULONG ret;
    ULONG err;

    Forbid();
    ret = Unit->hdskBase->iface->ReadFile(Unit->file, buf, size, &resSize, NULL);
    err = Unit->hdskBase->iface->GetLastError();
    Permit();

    if (ret)
	return resSize;

    *ioerr = error(err);
    return -1;
}

LONG Host_Write(struct unit *Unit, APTR buf, ULONG size, ULONG *ioerr)
{
    ULONG resSize;
    ULONG ret;
    ULONG err;

    Forbid();
    ret = Unit->hdskBase->iface->WriteFile(Unit->file, buf, size, &resSize, NULL);
    err = Unit->hdskBase->iface->GetLastError();
    Permit();

    if (ret)
	return resSize;

    *ioerr = error(err);
    return -1;
}

ULONG Host_Seek(struct unit *Unit, ULONG pos)
{
    ULONG ret;

    Forbid();
    ret = Unit->hdskBase->iface->SetFilePointer(Unit->file, pos, NULL, FILE_BEGIN);
    Permit();

    if (ret != -1)
	return 0;

    return TDERR_SeekError;
}

ULONG Host_GetGeometry(struct unit *Unit, struct DriveGeometry *dg)
{
    ULONG len, err;

    Forbid();
    len = Unit->hdskBase->iface->GetFileSize(Unit->file, NULL);
    err = Unit->hdskBase->iface->GetLastError();
    Permit();

    if (len == -1)
	return error(err);

    dg->dg_SectorSize = 512;
    dg->dg_Heads = 16;
    dg->dg_TrackSectors = 63;
    dg->dg_TotalSectors = len / dg->dg_SectorSize;
    /* in case of links or block devices with emul_handler we get the wrong size */
    if (dg->dg_TotalSectors == 0)
	dg->dg_TotalSectors = dg->dg_Heads*dg->dg_TrackSectors*5004;
    dg->dg_Cylinders = dg->dg_TotalSectors / (dg->dg_Heads * dg->dg_TrackSectors);
    dg->dg_CylSectors = dg->dg_Heads * dg->dg_TrackSectors;
    dg->dg_BufMemType = MEMF_PUBLIC;
    dg->dg_DeviceType = DG_DIRECT_ACCESS;
    dg->dg_Flags = DGF_REMOVABLE;

    return 0;
}

static const char *KernelSymbols[] = {
    "CreateFileA",
    "CloseHandle",
    "ReadFile",
    "WriteFile",
    "SetFilePointer",
    "GetFileAttributesA",
    "GetFileSize",
    "GetLastError",
    NULL
};

static int Host_Init(struct HostDiskBase *hdskBase)
{
    ULONG r;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[hostdisk] HostLibBase: 0x%p\n", HostLibBase));
    if (!HostLibBase)
	return FALSE;

    hdskBase->KernelHandle = HostLib_Open("kernel32.dll", NULL);
    if (!hdskBase->KernelHandle)
	return FALSE;

    hdskBase->iface = (struct HostInterface *)HostLib_GetInterface(hdskBase->KernelHandle, KernelSymbols, &r);
    if ((!hdskBase->iface) || r)
	return FALSE;

    hdskBase->DiskDevice = "\\\\.\\PhysicalDrive%ld";
	
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

ADD2INITLIB(Host_Init, 0)
ADD2EXPUNGELIB(Host_Cleanup, 0)

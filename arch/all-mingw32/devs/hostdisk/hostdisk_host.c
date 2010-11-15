#include <devices/trackdisk.h>
#include <proto/exec.h>

#include "hostdisk_host.h"
#include "hostdisk_device.h"

static ULONG error(ULONG winerr)
{
    switch(error)
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
    APTR file;
    ULONG attrs;

    Forbid();
    attrs = HostIf->GetFileAttributes(unit->filename);
    unit->file = HostIf->CreateFile(unit->filename, GENERIC_READ, FILE_SHARE_VALID_FLAGS, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    Permit();

    if (file == INVALIE_HANDLE_VALUE)
	return TDERR_NotSpecified;

    unit->writtable = !(attrs & FILE_ATTRIBUTE_READONLY);

    return 0;
}

void Host_Close(APTR f, struct HostInterface *HostIf)
{
    Forbid();
    HostIf->CloseHandle(f);
    Permit();
}

LONG Host_Read(APTR file, APTR buf, ULONG size, ULONG *ioerr, struct HostInterface *HostIf)
{
    ULONG resSize;
    ULONG ret;
    ULONG err;

    Forbid();
    ret = HostIf->ReadFile(file, buf, size, &resSize, NULL);
    err = HostIf->GetLastError();
    Permit();

    if (ret)
	return resSize;

    *ioerr = error(err);
    return -1;
}

LONG Host_Write(APTR file, APTR buf, ULONG size, ULONG *ioerr, struct HostInterface *HostIf)
{
    ULONG resSize;
    ULONG ret;
    ULONG err;

    Forbid();
    ret = HostIf->WriteFile(file, buf, size, &resSize, NULL);
    err = HostIf->GetLastError();
    Permit();

    if (ret)
	return resSize;

    *ioerr = error(err);
    return -1;
}

ULONG Host_Seek(APTR file, ULONG pos, struct HostInterface *HostIf)
{
    ULONG ret;

    Forbid();
    ret = HostIf->SetFilePointer(file, pos, NULL, FILE_BEGIN);
    Permit();

    if (ret != INVALID_SET_FILE_POINTER)
	return 0;

    return TDERR_SeekError;
}

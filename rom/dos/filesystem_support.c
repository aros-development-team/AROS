/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "dos_intern.h"

#include <string.h>


struct Device *GetDosType(ULONG type, CONST_STRPTR name, struct Unit **unit,
			  struct DosLibrary *DOSBase);


void InitIOFS(struct IOFileSys *iofs, ULONG type,
	      struct DosLibrary *DOSBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);

    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort    = &me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length       = sizeof(struct IOFileSys);
    iofs->IOFS.io_Command                 = type;
    iofs->IOFS.io_Flags                   = 0;
}

struct Device *GetDevice(CONST_STRPTR name, struct Unit **unit, 
			 struct DosLibrary *DOSBase)
{
    return GetDosType(LDF_DEVICES, name, unit, DOSBase);
//    return GetDosType(DLT_DEVICE, name, unit, DOSBase);
}


struct Device *GetVolume(CONST_STRPTR name, struct Unit **unit,
			 struct DosLibrary *DOSBase)
{
    return GetDosType(LDF_VOLUMES, name, unit, DOSBase);
//    return GetDosType(DLT_VOLUME, name, unit, DOSBase);
}


/* Return the device corresponding to the 'name'. This function will
   only look into "real" devices, that is no "PROGDIR:" or such.
   The pointer to the device unit will be written into 'unit' if
   'unit' is not NULL. */ 
struct Device *GetDosType(ULONG type, CONST_STRPTR name, struct Unit **unit,
			  struct DosLibrary *DOSBase)
{
    int     len = strlen(name);
    int     size;
    STRPTR  colon = strchr(name, ':');
    STRPTR  tempName;

    struct Device  *device = NULL;
    struct DosList *dl;

    if (colon == NULL)
    {
	return NULL;
    }

    size = colon - name;

    /* Not only a device name with trailing colon? */
    if (size + 1 != len)
    {
	return NULL;
    }

    tempName = AllocVec(len, MEMF_ANY);

    if (tempName == NULL)
    {
	return NULL;
    }

    CopyMem(name, tempName, size);
    tempName[size] = 0;		/* Terminate string */

    dl = LockDosList(type | LDF_READ);
    dl = FindDosEntry(dl, tempName, type);
    if (dl != NULL)
    {
	device = dl->dol_Ext.dol_AROS.dol_Device;
	if (unit!=NULL)
	    *unit = dl->dol_Ext.dol_AROS.dol_Unit;
    }

    UnLockDosList(type | LDF_READ);

    if (device == NULL)
    {
	SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
    }

    FreeVec(tempName);

    return device;
}

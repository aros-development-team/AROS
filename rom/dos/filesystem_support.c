/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
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

CONST_STRPTR StripVolume(CONST_STRPTR name) {
    const char *path = strchr(name, ':');
    if (path != NULL)
        path++;
    else
        path = name;
    return path;
}

LONG DoIOFS(struct IOFileSys *iofs, struct DevProc *dvp, CONST_STRPTR name,
    struct DosLibrary *DOSBase) {
    BOOL freedvp = FALSE;

    if (dvp == NULL) {
        if ((dvp = GetDeviceProc(name, NULL)) == NULL)
            return IoErr();

        freedvp = TRUE;
    }

    iofs->IOFS.io_Device = (struct Device *) dvp->dvp_Port;

    if (dvp->dvp_Lock != NULL)
        iofs->IOFS.io_Unit = ((struct FileHandle *) BADDR(dvp->dvp_Lock))->fh_Unit;
    else
        iofs->IOFS.io_Unit = dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit;

    if (name != NULL)
        iofs->io_Union.io_NamedFile.io_Filename = StripVolume(name);

    DosDoIO((struct IORequest *)iofs);

    SetIoErr(iofs->io_DosError);

    if (freedvp)
        FreeDeviceProc(dvp);

    return iofs->io_DosError;
}

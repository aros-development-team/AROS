/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Check if a device is a filesystem.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, IsFileSystem,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, devicename, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 118, Dos)

/*  FUNCTION
	Query the device whether it is a filesystem.

    INPUTS
	devicename	- Name of the device to query.

    RESULT
	TRUE if the device is a filesystem, FALSE otherwise.

    NOTES
	DF0:, HD0:, ... are filesystems.
	CON:, PIPE:, AUX:, ... are not

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);
    struct DosList *dl;
    BOOL success=0;
    

    dl=LockDosList(LDF_DEVICES|LDF_READ);
    dl=FindDosEntry(dl,devicename,LDF_DEVICES);
    if(dl!=NULL)
    {

        /* Get pointer to I/O request. Use stackspace for now. */
        struct IOFileSys io,*iofs=&io;

        /* Prepare I/O request. */
        iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
        iofs->IOFS.io_Message.mn_ReplyPort   =&me->pr_MsgPort;
        iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
        iofs->IOFS.io_Device =dl->dol_Device;
        iofs->IOFS.io_Unit   =dl->dol_Unit;
        iofs->IOFS.io_Command=FSA_IS_FILESYSTEM;
        iofs->IOFS.io_Flags  =0;

        /* Send the request. */
        DoIO(&iofs->IOFS);
        
        /* Set return code */
        if(!iofs->io_DosError) {
            success=iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem;
	}
    }
    /* All Done. */
    UnLockDosList(LDF_DEVICES|LDF_READ);
    return success;
    AROS_LIBFUNC_EXIT
} /* IsFilesystem */

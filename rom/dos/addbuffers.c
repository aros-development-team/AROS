/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Add or remove cache memory from a filesystem.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, AddBuffers,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, devicename, D1),
	AROS_LHA(LONG,         numbuffers, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 122, Dos)

/*  FUNCTION
	Add or remove cache memory from a filesystem.

    INPUTS
	devicename - NUL terminated dos device name.
	numbuffers - Number of buffers to add. May be negative.

    RESULT
	!=0 on success (IoErr() gives the actual number of buffers),
	0 else (IoErr() gives the error code).

    NOTES

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
        iofs->IOFS.io_Command=FSA_MORE_CACHE;
        iofs->IOFS.io_Flags  =0;
        iofs->io_Union.io_MORE_CACHE.io_NumBuffers=numbuffers;

        /* Send the request. */
        DoIO(&iofs->IOFS);
        
        /* Set error code */
        if(!iofs->io_DosError)
        {
            SetIoErr(iofs->io_Union.io_MORE_CACHE.io_NumBuffers);
            success=1; 
	}else
	    SetIoErr(iofs->io_DosError);
    }else
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
    /* All Done. */
    UnLockDosList(LDF_DEVICES|LDF_READ);
    return success;
    AROS_LIBFUNC_EXIT
} /* AddBuffers */

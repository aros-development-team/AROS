/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/11 12:54:45  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH3(BOOL, Format,

/*  SYNOPSIS */
	__AROS_LHA(STRPTR, devicename, D1),
	__AROS_LHA(STRPTR, volumename, D2),
	__AROS_LHA(ULONG,  dostype,    D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 119, Dos)

/*  FUNCTION

    INPUTS

    RESULT

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

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
        iofs->IOFS.io_Command=FSA_FORMAT;
        iofs->IOFS.io_Flags  =0;
        iofs->io_Args[0]=(ULONG)volumename;
        iofs->io_Args[1]=dostype;

        /* Send the request. */
        DoIO(&iofs->IOFS);
        
        /* Set error code */
        if(!iofs->io_DosError)
            success=1; 
	else
	    me->pr_Result2=iofs->io_DosError;
    }else
        me->pr_Result2=ERROR_DEVICE_NOT_MOUNTED;
    /* All Done. */
    UnLockDosList(LDF_DEVICES|LDF_READ);
    return success;
    __AROS_FUNC_EXIT
} /* Format */

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/13 13:52:45  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced __AROS_LA by __AROS_LHA

    Revision 1.2  1996/08/01 17:40:50  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH5(BOOL, ExAll,

/*  SYNOPSIS */
	__AROS_LHA(BPTR,                  lock,    D1),
	__AROS_LHA(struct ExAllData *,    buffer,  D2),
	__AROS_LHA(LONG,                  size,    D3),
	__AROS_LHA(LONG,                  data,    D4),
	__AROS_LHA(struct ExAllControl *, control, D5),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 72, Dos)

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

    /* Get pointer to filehandle */
    struct FileHandle *fh=(struct FileHandle *)BADDR(lock);

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort	 =&me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length	 =sizeof(struct IOFileSys);
    iofs->IOFS.io_Device =fh->fh_Device;
    iofs->IOFS.io_Unit	 =fh->fh_Unit;
    iofs->IOFS.io_Command=FSA_EXAMINE_ALL;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Args[0]=(LONG)buffer;
    iofs->io_Args[1]=size;
    iofs->io_Args[2]=data;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    /* Set error code and return */
    if((me->pr_Result2=iofs->io_DosError)!=0)
    {
	control->eac_Entries=0;
	return 0;
    }

    for(size=1;buffer!=NULL;size++)
    {
	buffer->ed_Prot^=0xf;
	buffer=buffer->ed_Next;
    }
    control->eac_Entries=size;

    return 1;
    __AROS_FUNC_EXIT
} /* ExAll */

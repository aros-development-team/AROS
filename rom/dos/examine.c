/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/10/24 15:50:27  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.5  1996/09/13 17:50:06  digulla
    Use IPTR

    Revision 1.4  1996/08/13 13:52:45  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.3  1996/08/12 14:20:38  digulla
    Added aliases

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

	AROS_LH2(BOOL, Examine,

/*  SYNOPSIS */
	AROS_LHA(BPTR,                   lock, D1),
	AROS_LHA(struct FileInfoBlock *, fib,  D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 17, Dos)

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

/*****************************************************************************

    NAME
	#include <clib/dos_protos.h>

	AROS_LH2(BOOL, ExamineFH,

    SYNOPSIS
	AROS_LHA(BPTR                  , fh, D1),
	AROS_LHA(struct FileInfoBlock *, fib, D2),

    LOCATION
	struct DosLibrary *, DOSBase, 65, Dos)

    FUNCTION

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
/*AROS alias ExamineFH Examine */
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    UBYTE buffer[512];
    struct ExAllData *ead=(struct ExAllData *)buffer;
    STRPTR src, dst;
    ULONG i;

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
    iofs->IOFS.io_Command=FSA_EXAMINE;
    iofs->IOFS.io_Flags  =0;
    iofs->io_Args[0]=(IPTR)buffer;
    iofs->io_Args[1]=512;
    iofs->io_Args[2]=ED_OWNER;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    /* Set error code and return */
    if((me->pr_Result2=iofs->io_DosError))
	return 0;
    else
    {
	fib->fib_DiskKey=0;
	fib->fib_DirEntryType=ead->ed_Type;
	src=ead->ed_Name;
	dst=fib->fib_FileName;
	if(src!=NULL)
	    for(i=0;i<107;i++)
		if(!(*dst++=*src++))
		    break;
	*dst++=0;
	fib->fib_Protection=ead->ed_Prot^0xf;
	fib->fib_EntryType=ead->ed_Type;
	fib->fib_Size=ead->ed_Size;
	fib->fib_Date.ds_Days=ead->ed_Days;
	fib->fib_Date.ds_Minute=ead->ed_Mins;
	fib->fib_Date.ds_Tick=ead->ed_Ticks;
	src=ead->ed_Comment;
	dst=fib->fib_Comment;
	if(src!=NULL)
	    for(i=0;i<79;i++)
		if(!(*dst++=*src++))
		    break;
	*dst++=0;
	fib->fib_OwnerUID=ead->ed_OwnerUID;
	fib->fib_OwnerGID=ead->ed_OwnerGID;
	return 1;
    }

    AROS_LIBFUNC_EXIT
} /* Examine */

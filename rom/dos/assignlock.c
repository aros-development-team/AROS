/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/10/24 15:50:24  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:44  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:47  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	AROS_LH2(BOOL, AssignLock,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(BPTR,   lock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 102, Dos)

/*  FUNCTION
	Create an assign from a given name to a lock. Replaces any older
	assignments from that name, 0 cancels the assign completely. Do not
	use or free the lock after calling this function - it becomes
	the assign and will be freed by the system if the assign is removed.

    INPUTS
	name - NUL terminated name of the assign.
	lock - Lock to assigned directory.

    RESULT
	!=0 success, 0 on failure. IoErr() gives additional information
	in that case. The lock is freed on failure and must not be used
	in that case too.

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

    BOOL success=1;
    struct DosList *dl, *newdl=NULL;
    struct Process *me=(struct Process *)FindTask(NULL);
    struct IOFileSys io,*iofs=&io;
    struct FileHandle *fh=(struct FileHandle *)BADDR(lock);

    if(lock)
    {
	newdl=MakeDosEntry(name,DLT_DIRECTORY);
	if(newdl==NULL)
	{
	    UnLock(lock);
	    me->pr_Result2=ERROR_NO_FREE_STORE;
	    return 0;
	}else
	{
	    newdl->dol_Unit  =fh->fh_Unit;
	    newdl->dol_Device=fh->fh_Device;
	    FreeDosObject(DOS_FILEHANDLE,fh);
	}
    }

    dl=LockDosList(LDF_DEVICES|LDF_ASSIGNS|LDF_WRITE);

    dl=FindDosEntry(dl,name,LDF_DEVICES|LDF_ASSIGNS);
    if(dl==NULL)
    {
	if(newdl!=NULL)
	    AddDosEntry(newdl);
    }else if(dl->dol_Type==DLT_DEVICE)
    {
	dl=newdl;
	me->pr_Result2=ERROR_OBJECT_EXISTS;
	success=0;
    }else
    {
	RemDosEntry(dl);
	if(newdl!=NULL)
	    AddDosEntry(newdl);
    }

    if(dl!=NULL)
    {
	/* Prepare I/O request. */
	iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
	iofs->IOFS.io_Message.mn_ReplyPort   =&me->pr_MsgPort;
	iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
	iofs->IOFS.io_Device =dl->dol_Device;
	iofs->IOFS.io_Unit   =dl->dol_Unit;
	iofs->IOFS.io_Command=FSA_CLOSE;
	iofs->IOFS.io_Flags  =0;

	/* Send the request. No errors possible. */
	(void)DoIO(&iofs->IOFS);

	FreeDosEntry(dl);
    }

    UnLockDosList(LDF_DEVICES|LDF_ASSIGNS|LDF_WRITE);

    return success;
    AROS_LIBFUNC_EXIT
} /* AssignLock */

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Locks a file or directory.
    Lang: english
*/
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BPTR, Lock,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name,       D1),
	AROS_LHA(LONG,   accessMode, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 14, Dos)

/*  FUNCTION
	Gets a lock on a file or directory. There may be more than one
	shared lock on a file but only one if it is an exclusive one.
	Locked files or directories may not be deleted.

    INPUTS
	name	   - NUL terminated name of the file or directory.
	accessMode - One of SHARED_LOCK
			    EXCLUSIVE_LOCK

    RESULT
	Handle to the file or directory or 0 if the object couldn't be locked.
	IoErr() gives additional information in that case.

    NOTES
	The lock structure returned by this function is different
	from that of AmigaOS (in fact it is identical to a filehandle).
	Do not try to read any internal fields.

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

    struct FileHandle *ret;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Create filehandle */
    ret=(struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,NULL);
    if(ret!=NULL)
    {
	/* Get pointer to I/O request. Use stackspace for now. */
	struct IOFileSys io,*iofs=&io;

	/* Prepare I/O request. */
	iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
	iofs->IOFS.io_Message.mn_ReplyPort   =&me->pr_MsgPort;
	iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
	iofs->IOFS.io_Flags=0;
	iofs->IOFS.io_Command=FSA_OPEN;
	/* io_Args[0] is the name which is set by DoName(). */
	switch(accessMode)
	{
	    case EXCLUSIVE_LOCK:
		iofs->io_Args[1]=FMF_LOCK|FMF_READ;
		break;
	    case SHARED_LOCK:
		iofs->io_Args[1]=FMF_READ;
		break;
	    default:
		iofs->io_Args[1]=accessMode;
		break;
	}
	if(!DoName(iofs,name,DOSBase))
	{
	    ret->fh_Device=iofs->IOFS.io_Device;
	    ret->fh_Unit  =iofs->IOFS.io_Unit;
	    return MKBADDR(ret);
	}
	FreeDosObject(DOS_FILEHANDLE,ret);
    }else
	SetIoErr(ERROR_NO_FREE_STORE);
    return 0;
    AROS_LIBFUNC_EXIT
} /* Lock */

/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1996/12/09 13:53:23  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.3  1996/10/24 15:50:25  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/09/21 14:14:22  digulla
    Hand DOSBase to DoName()

    Revision 1.1  1996/09/11 12:54:45  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <clib/dos_protos.h>

	AROS_LH1(BPTR, CreateDir,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 20, Dos)

/*  FUNCTION
	Creates a new directory under the given name. If all went an
	exclusive lock on the new diretory is returned.

    INPUTS
	name	   - NUL terminated name.

    RESULT
	Exclusive lock to the new directory or 0 if couldn't be created.
	IoErr() gives additional information in that case.

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

    struct FileHandle *ret;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys io,*iofs=&io;

    /* Allocate memory for lock */
    ret=(struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,NULL);
    if(ret!=NULL)
    {
	/* Prepare I/O request. */
	iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
	iofs->IOFS.io_Message.mn_ReplyPort   =&me->pr_MsgPort;
	iofs->IOFS.io_Message.mn_Length      =sizeof(struct IOFileSys);
	iofs->IOFS.io_Flags=0;
	iofs->IOFS.io_Command=FSA_CREATE_DIR;
	/* io_Args[0] is the name which is set by DoName(). */
	iofs->io_Args[1]=FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE;
	if(!DoName(iofs,name,DOSBase))
	{
	    ret->fh_Unit  =iofs->IOFS.io_Unit;
	    ret->fh_Device=iofs->IOFS.io_Device;
	    return MKBADDR(ret);
	}
	FreeDosObject(DOS_FILEHANDLE,ret);
    }else
	me->pr_Result2=ERROR_NO_FREE_STORE;
    return 0;
    AROS_LIBFUNC_EXIT
} /* CreateDir */

/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Open a file with the specified mode.
    Lang: english
*/
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BPTR, Open,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,       D1),
	AROS_LHA(LONG,         accessMode, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 5, Dos)

/*  FUNCTION
	Opens a file for read and/or write depending on the accessmode given.

    INPUTS
	name	   - NUL terminated name of the file.
	accessMode - One of MODE_OLDFILE   - open existing file
			    MODE_NEWFILE   - delete old, create new file
					     exclusive lock
			    MODE_READWRITE - open new one if it doesn't exist

    RESULT
	Handle to the file or 0 if the file couldn't be opened.
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
    BPTR con, ast;
    LONG error;

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
	iofs->IOFS.io_Command=FSA_OPEN_FILE;

	/* io_Args[0] is the name which is set by DoName(). */
	switch(accessMode)
	{
	    case MODE_OLDFILE:
		iofs->io_Union.io_OPEN_FILE.io_FileMode=FMF_WRITE|FMF_READ;
		ast=con=me->pr_CIS;
		break;
	    case MODE_NEWFILE:
		iofs->io_Union.io_OPEN_FILE.io_FileMode=FMF_LOCK|FMF_CREATE|FMF_CLEAR|FMF_WRITE|FMF_READ;
		con=me->pr_COS;
		ast=me->pr_CES?me->pr_CES:me->pr_COS;
		break;
	    case MODE_READWRITE:
		iofs->io_Union.io_OPEN_FILE.io_FileMode=FMF_CREATE|FMF_WRITE|FMF_READ;
		con=me->pr_COS;
		ast=me->pr_CES?me->pr_CES:me->pr_COS;
		break;
	    default:
		iofs->io_Union.io_OPEN_FILE.io_FileMode=accessMode;
		ast=con=me->pr_CIS;
		break;
	}
	iofs->io_Union.io_OPEN_FILE.io_Protection=0UL;
	if(!Stricmp(name,"CONSOLE:"))
	{
	    iofs->IOFS.io_Device=((struct FileHandle *)BADDR(con))->fh_Device;
	    iofs->IOFS.io_Unit	=((struct FileHandle *)BADDR(con))->fh_Unit;
	    iofs->io_Union.io_OPEN_FILE.io_Filename="";
	    (void)DoIO(&iofs->IOFS);
	    error=me->pr_Result2=iofs->io_DosError;
	}else if(!Stricmp(name,"*"))
	{
	    iofs->IOFS.io_Device=((struct FileHandle *)BADDR(ast))->fh_Device;
	    iofs->IOFS.io_Unit	=((struct FileHandle *)BADDR(ast))->fh_Unit;
	    iofs->io_Union.io_OPEN_FILE.io_Filename="";
	    (void)DoIO(&iofs->IOFS);
	    error=me->pr_Result2=iofs->io_DosError;
	}else
	    error=DoName(iofs,name,DOSBase);
	if(!error)
	{
	    ret->fh_Device=iofs->IOFS.io_Device;
	    ret->fh_Unit  =iofs->IOFS.io_Unit;
	    return MKBADDR(ret);
	}
	FreeDosObject(DOS_FILEHANDLE,ret);
    }else
	me->pr_Result2=ERROR_NO_FREE_STORE;
    return 0;
    AROS_LIBFUNC_EXIT
} /* Open */

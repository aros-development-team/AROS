/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/12 14:20:38  digulla
    Added aliases

    Revision 1.2  1996/08/01 17:40:55  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/exall.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH3(BOOL, NameFromLock,

/*  SYNOPSIS */
	__AROS_LA(BPTR,   lock,   D1),
	__AROS_LA(STRPTR, buffer, D2),
	__AROS_LA(LONG,   length, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 67, Dos)

/*  FUNCTION
	Get the full path name associated with a lock to a file or
	directory into a user supplied buffer.

    INPUTS
	lock   - Lock to file or directory.
	buffer - Buffer to fill. Contains a NUL terminated string if
		 all went well.
	length - Size of the buffer in bytes.

    RESULT
	!=0 if all went well, 0 in case of an error. IoErr() will
	give additional information in that case.

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

	__AROS_LH3(LONG, NameFromFH,

    SYNOPSIS
	__AROS_LA(BPTR  , fh, D1),
	__AROS_LA(STRPTR, buffer, D2),
	__AROS_LA(long  , len, D3),

    LOCATION
	struct DosLibrary *, DOSBase, 68, Dos)

    FUNCTION
	Get the full path name associated with file-handle into a
	user supplied buffer.

    INPUTS
	fh     - File-handle to file or directory.
	buffer - Buffer to fill. Contains a NUL terminated string if
		 all went well.
	length - Size of the buffer in bytes.

    RESULT
	!=0 if all went well, 0 in case of an error. IoErr() will
	give additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
/*AROS alias NameFromFH NameFromLock */
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    STRPTR s1, s2, name;
    struct Unit *curlock, *oldlock=NULL;
    struct ExAllData *ead=(struct ExAllData *)buffer;
    LONG error;

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
    iofs->IOFS.io_Device= fh==NULL?DOSBase->dl_NulHandler:fh->fh_Device;

    /* Construct the name from top to bottom */
    name=buffer+length;
    *--name=0;
    curlock= fh==NULL?DOSBase->dl_NulLock:fh->fh_Unit;
    /* Loop over path */
    do
    {
	/* Read name of current lock (into the user supplied buffer) */
	iofs->IOFS.io_Unit=curlock;
	iofs->IOFS.io_Command=FSA_EXAMINE;
	iofs->io_Args[0]=(LONG)buffer;
	iofs->io_Args[1]=name-buffer;
	iofs->io_Args[2]=ED_TYPE;
	DoIO(&iofs->IOFS);
	error=iofs->io_DosError;

	/* Move name to the top of the buffer. */
	if(!error)
	{
	    s1=s2=ead->ed_Name;
	    while(*s2++)
		;
	    if(ead->ed_Type==ST_ROOT)
		*--name=':';
	    else if(oldlock!=NULL)
		*--name='/';
	    s2--;
	    while(s2>s1)
		*--name=*--s2;
	}

	/* Read the parent's lock (if there is a parent) */
	if(!error&&ead->ed_Type!=ST_ROOT)
	{
	    iofs->IOFS.io_Command=FSA_OPEN;
	    iofs->io_Args[0]=(LONG)"/";
	    iofs->io_Args[1]=0;
	    DoIO(&iofs->IOFS);
	    curlock=iofs->IOFS.io_Unit;
	    error=iofs->io_DosError;
	}

	/* Free the old lock if it was allocated by NameFromLock(). */
	if(oldlock!=NULL)
	{
	    iofs->IOFS.io_Unit=oldlock;
	    iofs->IOFS.io_Command=FSA_CLOSE;
	    DoIO(&iofs->IOFS);
	}
	oldlock=curlock;
    }while(!error&&ead->ed_Type!=ST_ROOT);

    /* Move the name from the top to the bottom of the buffer. */
    while((*buffer++=*name++)!=0)
	;
    /* All done. */
    me->pr_Result2=error;
    return !error;
    __AROS_FUNC_EXIT
} /* NameFromLock */

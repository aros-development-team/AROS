/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Retrieve thew full pathname from a lock.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/exall.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(BOOL, NameFromLock,

/*  SYNOPSIS */
	AROS_LHA(BPTR,   lock,   D1),
	AROS_LHA(STRPTR, buffer, D2),
	AROS_LHA(LONG,   length, D3),

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

	AROS_LH3(LONG, NameFromFH,

    SYNOPSIS
	AROS_LHA(BPTR  , fh, D1),
	AROS_LHA(STRPTR, buffer, D2),
	AROS_LHA(LONG  , len, D3),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    STRPTR s1, s2, name;
    struct Unit *curlock, *oldlock=NULL;
    struct ExAllData stackead;
    struct ExAllData *ead = &stackead;
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
	iofs->io_Union.io_EXAMINE.io_ead =ead;
	iofs->io_Union.io_EXAMINE.io_Size=sizeof(stackead);
	iofs->io_Union.io_EXAMINE.io_Mode=ED_TYPE;
	DosDoIO(&iofs->IOFS);
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
	    iofs->io_Union.io_OPEN.io_Filename="/";
	    iofs->io_Union.io_OPEN.io_FileMode=0;
	    DosDoIO(&iofs->IOFS);
	    curlock=iofs->IOFS.io_Unit;
	    error=iofs->io_DosError;
	}

	/* Free the old lock if it was allocated by NameFromLock(). */
	if(oldlock!=NULL)
	{
	    iofs->IOFS.io_Unit=oldlock;
	    iofs->IOFS.io_Command=FSA_CLOSE;
	    DosDoIO(&iofs->IOFS);
	}
	oldlock=curlock;
    }while(!error&&ead->ed_Type!=ST_ROOT);

    /* Move the name from the top to the bottom of the buffer. */
    {
        UBYTE c, old_c = '\0';
	
	do
	{
	    c = *name++;
	    if ((c != '/') || (old_c != ':'))
	    {
	        *buffer++ = c;
	    }
	    old_c = c;
	} while (c);
    }
     
    /* All done. */
    me->pr_Result2=error;
    
    return error ? DOSFALSE : DOSTRUE;
    
    AROS_LIBFUNC_EXIT
} /* NameFromLock */

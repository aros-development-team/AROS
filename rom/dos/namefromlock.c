/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Retrieve the full pathname from a lock.
    Lang: english
*/
#include <proto/exec.h>
#include <dos/exall.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

#include <aros/debug.h>

struct MyExAllData
{
    struct ExAllData ead;
    UBYTE   	     filenamebuffer[MAXFILENAMELENGTH + 1];
};

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

*****************************************************************************/
/*AROS alias NameFromFH NameFromLock */
{
    AROS_LIBFUNC_INIT

    STRPTR  	    	 s1, s2, name;
    struct Unit     	*curlock, *parentlock;
    struct MyExAllData   stackead;
    struct ExAllData 	*ead = &stackead.ead;
    LONG    	    	 error;

    /* Get pointer to filehandle */
    struct FileHandle 	*fh = (struct FileHandle *)BADDR(DupLock(lock));

    /* Get pointer to process structure */
    struct Process  	*me = (struct Process *)FindTask(NULL);

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys 	 io, *iofs = &io;

    if (fh == 0)
	return DOSFALSE;
	
    if (length < 1)
    {
        SetIoErr(ERROR_LINE_TOO_LONG);

        return DOSFALSE;
    }
    
    /* Prepare I/O request. */
    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort	  = &me->pr_MsgPort;
    iofs->IOFS.io_Message.mn_Length	  = sizeof(struct IOFileSys);

    iofs->IOFS.io_Device = (fh == NULL)
                                ? DOSBase->dl_NulHandler
                                : fh->fh_Device;

    /* Construct the name from top to bottom */
    name = buffer + length;
    *--name = 0;
    parentlock = fh->fh_Unit;

    iofs->io_DirPos = -1;
    /* Loop over path */
    do
    {
    	STRPTR sep = NULL;

   	curlock = parentlock;
    	parentlock = NULL;

    	/* Read name of current lock (into the user supplied buffer) */
    	iofs->IOFS.io_Unit  	    	    = curlock;
    	iofs->IOFS.io_Command	    	    = FSA_EXAMINE;
    	iofs->io_Union.io_EXAMINE.io_ead    = ead;
    	iofs->io_Union.io_EXAMINE.io_Size   = sizeof(stackead);
    	iofs->io_Union.io_EXAMINE.io_Mode   = ED_TYPE;
    	DosDoIO(&iofs->IOFS);
    	error = iofs->io_DosError;

    	/* Read the parent's lock (if there is a parent) */
    	if(!error && ead->ed_Type != ST_ROOT)
    	{
    	    iofs->IOFS.io_Command   	    	= FSA_OPEN;
    	    iofs->io_Union.io_OPEN.io_Filename	= "/";
    	    iofs->io_Union.io_OPEN.io_FileMode	= 0;
    	    DosDoIO(&iofs->IOFS);
    	    error = iofs->io_DosError;
	    if (!error)
	    {
		parentlock = iofs->IOFS.io_Unit;

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    	    /* Some 'classic' filesystems don't ever return ST_ROOT.
    	     * We check to see if the lock is the same as the previous
    	     * lock, and if so, assume we're reached the root.
    	     */
    	        iofs->IOFS.io_Unit = curlock;
    	        iofs->IOFS.io_Command = FSA_SAME_LOCK;
    	        iofs->io_Union.io_SAME_LOCK.io_Lock[0] = curlock;
    	        iofs->io_Union.io_SAME_LOCK.io_Lock[1] = parentlock;
    	        iofs->io_Union.io_SAME_LOCK.io_Same = LOCK_DIFFERENT;
    	        DosDoIO(&iofs->IOFS);
    	        if (iofs->io_DosError == 0 && iofs->io_Union.io_SAME_LOCK.io_Same == LOCK_SAME)
    	    	   ead->ed_Type = ST_ROOT;
#endif
    	    }
    	}
    	/* Move name to the top of the buffer. */
    	if(!error)
    	{
    	    s1 = s2 = ead->ed_Name;
    
    	    while(*s2++)
            {
                ;
            }
    		
    	    if(ead->ed_Type == ST_ROOT)
    	    {
                if (name > buffer)
                {
                    *--name=':';
                }
                else
                {
                    error = ERROR_LINE_TOO_LONG;
                }
    	    }
    	    else if(curlock != fh->fh_Unit)
    	    {
                if (name > buffer)
                {		    
                    *--name = '/';
                }
                else
                {
                    error = ERROR_LINE_TOO_LONG;
                }
    	    }

    	    /* Stash away the location of the directory separator */
    	    sep = name;
    	    
    	    if (!error)
    	    {
        		s2--;
        
        		if (name - (s2 - s1) >= buffer)
        		{ 
        	    	    while(s2 > s1)
                        {
                            *--name = *--s2;
                        }
        		}
        		else
        		{
        	    	    error = ERROR_LINE_TOO_LONG;
        		}
    	    }
    	    
    	} /* if(!error) */
       	/* Free the old lock if it was allocated by NameFromLock(). */
    	if(curlock != fh->fh_Unit)
    	{
    	    iofs->IOFS.io_Unit	    = curlock;
    	    iofs->IOFS.io_Command   = FSA_CLOSE;
    	    DosDoIO(&iofs->IOFS);
    	}
     	
    }
    while(!error && ead->ed_Type != ST_ROOT);

    if(parentlock)
    {
        iofs->IOFS.io_Unit	= parentlock;
        iofs->IOFS.io_Command   = FSA_CLOSE;
        DosDoIO(&iofs->IOFS);
    }

    /* Move the name from the top to the bottom of the buffer. */
    if (!error)
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
    	    
    	}
        while (c);
    }
    UnLock((BPTR)MKBADDR(fh));

    /* All done. */

    SetIoErr(error);

    return error
        ? DOSFALSE
        : DOSTRUE;
    
    AROS_LIBFUNC_EXIT
    
} /* NameFromLock */

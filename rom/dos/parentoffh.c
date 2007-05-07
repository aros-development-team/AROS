/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Lock the directory a file is located in
    Lang: english
*/
#include "dos_intern.h"
#include <dos/dosasl.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BPTR, ParentOfFH,

/*  SYNOPSIS */
	AROS_LHA(BPTR, fh, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 64, Dos)

/*  FUNCTION
	Lock the directory a file is located in.

    INPUTS
	fh  - Filhandle of which you want to obtain the parent
	
    RESULT
	lock - Lock on the parent directory of the filehandle or
	       NULL for failure.
	       

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	Lock(), UnLock(), Parent() 
	
    INTERNALS

*****************************************************************************/
{  
    AROS_LIBFUNC_INIT

    BPTR    lock = MKBADDR(NULL); 
    LONG    success = DOSFALSE;
    char *  Buffer = NULL;
    LONG    Buffersize = 256;
    LONG    Attempts = 1;

    #define DEF_BUFSIZESTEP 128

    if (fh != NULL)
    {
	/* Attempt to get the string of the file fh */
	while ((DOSFALSE == success) && (Attempts <= 5))
	{
	    Buffer = (char *) AllocMem(Buffersize, MEMF_CLEAR);

	    if (NULL ==  Buffer)
              	return NULL;

	    success = NameFromFH(fh, Buffer, Buffersize);

	    /* did it fail with a buffer overflow?? */
	    if (DOSFALSE == success)
	    {
        	if (ERROR_BUFFER_OVERFLOW == IoErr())
        	{
        	    Attempts++;
        	    FreeMem(Buffer, Buffersize);
        	    Buffersize += DEF_BUFSIZESTEP;
        	}
        	else /* another error occured -> exit */
        	{
        	    FreeMem(Buffer, Buffersize);
        	    return NULL;
        	}
	    }
	  
	} /* while ((DOSFALSE == success) && (Attempts <= 5)) */
	
	if (DOSTRUE == success)
	{
	    char * PP = PathPart(Buffer); /* get the path part of the file */

	    *PP = '\0';
	    
	    lock = Lock (Buffer, SHARED_LOCK);
	}
	
	FreeMem(Buffer, Buffersize);

    } /* if (fh != NULL) */

    return lock;

    AROS_LIBFUNC_EXIT
  
} /* ParentOfFH */

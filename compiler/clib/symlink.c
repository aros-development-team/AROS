/*
    Copyright © 2004-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function symlink().
*/

#include <aros/debug.h>

#include <stdlib.h>
#include <proto/dos.h>
#include <errno.h>
#include "__upath.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int symlink(

/*  SYNOPSIS */
	const char *oldpath,
	const char *newpath)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int retval = -1;
    BPTR lock;
    LONG ioerr = 0;
    UBYTE *buffer;
    int buffersize = 256;

    if (!oldpath || !newpath) /*safety check */
    {
        errno = EFAULT;
        return -1;
    }

    oldpath = __path_u2a(oldpath);
    if(!oldpath)
	return -1;
    
    oldpath = strdup(oldpath);
    if(!oldpath)
    {
	errno = ENOMEM;
	return -1;
    }

    newpath = __path_u2a(newpath);
    if (!newpath)
    {
	free((void*) oldpath);
	return -1;
    }
	
    if((lock = Lock((STRPTR)oldpath, SHARED_LOCK))) 
    {
	do
	{
	    if(!(buffer = AllocVec(buffersize, MEMF_ANY)))
	    {
		ioerr = ERROR_NO_FREE_STORE;
		break;
	    }
				
	    /* Get the full path of oldpath */
	    if(NameFromLock(lock, buffer, buffersize))
	    {
		if(MakeLink((STRPTR)newpath, (STRPTR)buffer, TRUE))
		    retval = 0;
		else
		{
		    ioerr = IoErr();
		    FreeVec(buffer);
		    break;
		}
	    }
	    else if(IoErr() != ERROR_LINE_TOO_LONG)
	    {
		ioerr = IoErr();
		FreeVec(buffer);
		break;
	    }
	    FreeVec(buffer);
	    buffersize *= 2;
	}
	while(retval != RETURN_OK);
	UnLock(lock);
    }
    else
    {
	/* MakeLink can create symlinks to non-existing files or 
	   directories. */
	if(IoErr() == ERROR_OBJECT_NOT_FOUND)
	{
	    /* In this case it may be difficult to get the full absolute 
	       path, so we simply trust the caller here for now */
	    if(MakeLink((STRPTR)newpath, (STRPTR)oldpath, TRUE))
		retval = 0;
	    else
		ioerr = IoErr();
	}
	else
	    ioerr = IoErr();		
    }
    free((void*) oldpath);

    if(ioerr)
	errno = __arosc_ioerr2errno(ioerr);

    return retval;
} /* symlink */


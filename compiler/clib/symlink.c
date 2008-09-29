/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function symlink().
*/

#include <aros/debug.h>

#include <stdlib.h>
#include <unistd.h>
#include <proto/dos.h>
#include <errno.h>
#include "__errno.h"
#include "__upath.h"
#include <aros/debug.h>

/*****************************************************************************

    NAME */

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
    struct FileInfoBlock *fib;
    int errorset = FALSE;
    LONG ioerr;
    UBYTE *buffer;
    int bufferincrease = 256;
    int buffersize = bufferincrease;

    if (!oldpath || !newpath) /*safety check */
    {
        errno = EFAULT;
        return -1;
    }

    if(oldpath = strdup(__path_u2a(oldpath)))
    {
	    newpath = __path_u2a(newpath);
	    if (!newpath)
	    {
	    	free(oldpath);
	    	return -1;
	    }
	
		if(lock = Lock((STRPTR)oldpath, SHARED_LOCK)) {
			do
			{
				if(!(buffer = AllocVec(buffersize, MEMF_ANY)))
		        {
					ioerr = ERROR_NO_FREE_STORE;
					errorset = TRUE;
		            break;
		        }
				
	    		/* Get the full path of oldpath */
		    	if(NameFromLock(lock, buffer, buffersize))
		    	{
					if(MakeLink((STRPTR)newpath,
						    (STRPTR)buffer,
						    TRUE) == DOSTRUE)
					    retval = RETURN_OK;
					else
					{
						ioerr = IoErr();
						errorset = TRUE;
						break;
					}
		    	}
		    	else if(IoErr() != ERROR_LINE_TOO_LONG)
				{
					ioerr = IoErr();
					errorset = TRUE;
					break;
				}
		    	FreeVec(buffer);
		    	buffersize += bufferincrease;
			}
			while(retval != RETURN_OK);
		    UnLock(lock);
		}
		else
		{
			ioerr = IoErr();
			/* I'm not sure if MakeLink is allowed to create symlinks to
			   non-existing files or directories. If yes, then it's fine to
			   enable the following code */
#if 0
			if(ioerr == ERROR_OBJECT_NOT_FOUND)
			{
				/* On Unices it's perfectly fine to create symlinks to
				   non-existing files or directories, however in this case it
				   may be difficult to get the full absolute path, so we are
				   simply trusting the user here for now */
				if(MakeLink((STRPTR)newpath,
					    (STRPTR)oldpath,
					    TRUE) == DOSTRUE)
				    retval = RETURN_OK;
				else
				{
					ioerr = IoErr();
					errorset = TRUE;
				}			
			}
			else
#endif
				errorset = TRUE;		
		}
		free(oldpath);
    }

	if(errorset) errno = IoErr2errno(ioerr);
    return retval;
} /* symlink */


/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function fchmod().
*/

#include <errno.h>

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <sys/types.h>
#include <stdlib.h>

#include "__fdesc.h"
#include "__upath.h"

ULONG prot_u2a(mode_t protect);

/*****************************************************************************

    NAME */
#include <sys/types.h>
#include <sys/stat.h>

	int fchmod (

/*  SYNOPSIS */
	int filedes,
	mode_t mode)

/*  FUNCTION
	Change permission bits of a file specified by an open file descriptor.

    INPUTS
	filedes - File descriptor of the file
	mode - Permission bits to set

    RESULT
	0 on success and -1 on error. If an error occurred, the global
	variable errno is set.

    NOTES
	See chmod() documentation for more details about the mode parameter.

    EXAMPLE

    BUGS

    SEE ALSO
	chmod()

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc;
    UBYTE *buffer;
    int buffersize = 256;

    if (!(fdesc = __getfdesc(filedes)))
    {
	errno = EBADF;
	return -1;
    }
    
    /* Get the full path of the stated filesystem object and use it to
       compute hash value */
    do
    {
        if(!(buffer = AllocVec(buffersize, MEMF_ANY)))
        {
            errno = __stdc_ioerr2errno(IoErr());
            return -1;
        }
            
        if(NameFromFH(fdesc->fcb->fh, buffer, buffersize))
            break;
        else if(IoErr() != ERROR_LINE_TOO_LONG)
        {
            errno = __stdc_ioerr2errno(IoErr());
            FreeVec(buffer);
            return -1;
        }
        FreeVec(buffer);
        buffersize *= 2;
    }
    while(TRUE);
    
    if (!SetProtection(buffer, prot_u2a(mode)))
    {
	FreeVec(buffer);
	errno = __stdc_ioerr2errno(IoErr());
	return -1;
    }

    FreeVec(buffer);
    return 0;
} /* fchmod */


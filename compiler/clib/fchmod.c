/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function fchmod().
*/

#include <errno.h>

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <sys/types.h>
#include <stdlib.h>

#include "__stdio.h"
#include "__open.h"
#include "__errno.h"
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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    fdesc *fdesc;
    UBYTE *buffer;
    int buffersize = 256;
    char *path;

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
            errno = IoErr2errno(IoErr());
            return -1;
        }
            
        if(NameFromLock(fdesc->fh, buffer, buffersize))
            break;
        else if(IoErr() != ERROR_LINE_TOO_LONG)
        {
            errno = IoErr2errno(IoErr());
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
	errno = IoErr2errno(IoErr());
	return -1;
    }

    FreeVec(buffer);
    return 0;
} /* fchmod */


/*
    Copyright © 2008-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__arosc_privdata.h"
#include "__fdesc.h"

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <errno.h>
#include "__upath.h"
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

        int fchdir(

/*  SYNOPSIS */
        int fd )

/*  FUNCTION
        Change the current working directory to the directory given as an open
        file descriptor.

    INPUTS
        fd - File descriptor of the directory to change to.
        
    RESULT
        If the current directory was changed successfully, zero is returned.
        Otherwise, -1 is returned and errno set apropriately.
        
    NOTES
        At program exit, the current working directory will be changed back
        to the one that was current when the program first started. If you
        do not desire this behaviour, use dos.library/CurrentDir() instead.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    BPTR oldlock = BNULL;
    BPTR newlock = BNULL;
    BPTR handle = BNULL;

    fdesc *fdc = __getfdesc(fd);
    if (!fdc->fcb->isdir)
    {
        errno = ENOTDIR;
        goto error; 
    }

    if ( __get_default_file(fd, (long*) &handle) != 0 )
    {
        errno = EBADF;
        goto error;
    }

    newlock = DupLock(handle);

    if( newlock == BNULL )
    {
        errno = __arosc_ioerr2errno( IoErr() );
        goto error;
    }
    oldlock = CurrentDir( newlock );

    if( aroscbase->acb_cd_changed )
    {
        UnLock( oldlock );
    }
    else
    {
        aroscbase->acb_cd_changed = TRUE;
        aroscbase->acb_cd_lock    = oldlock;
    }       
    return 0;

error:
    if( newlock != BNULL )
        UnLock( newlock );
    
    return -1;
}

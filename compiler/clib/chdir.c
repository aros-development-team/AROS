/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function chdir().
*/

#include "__arosc_privdata.h"

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <errno.h>
#include "__upath.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

	int chdir(

/*  SYNOPSIS */
	const char *path )

/*  FUNCTION
	Change the current working directory to the one specified by path.

    INPUTS
    	path - Path of the directory to change to.
	
    RESULT
	If the current directory was changed successfully, zero is returned.	
    	Otherwise, -1 is returned and errno set apropriately.
	
    NOTES
    	At program exit, the current working directory will be changed back
	to the one that was current when the program first started. If you
	do not desire this behaviour, use dos.library/CurrentDir() instead.
        The path given to chdir can be translated so that getcwd gives back
        a string that is not the same but points to th same directory. For
        example, assigns are replaced by the path where the assign points to
        and device names (like DH0:) are replaced with the volume name
        (e.g. Workbench:).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    BPTR oldlock;
    BPTR newlock;
    
    path = __path_u2a(path);
    
    if (path == NULL)
        return -1;
	    
    newlock = Lock( path, SHARED_LOCK );

    if( newlock == BNULL )
    {
        errno = __stdc_ioerr2errno( IoErr() );
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
    if( newlock != BNULL ) UnLock( newlock );
    
    return -1;
}

int __init_chdir(struct aroscbase *aroscbase)
{
    aroscbase->acb_cd_changed = FALSE;

    return 1;
}

void __exit_chdir(struct aroscbase *aroscbase)
{
    if( aroscbase->acb_cd_changed )
    {
        BPTR lock = CurrentDir( aroscbase->acb_cd_lock );

        UnLock( lock );
    }
}

ADD2OPENLIB(__init_chdir, -100);
ADD2CLOSELIB(__exit_chdir, -100);

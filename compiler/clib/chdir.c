/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved. 
    $Id$

    Desc: ANSI C function chdir()
    Lang: English
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <errno.h>
#include "__errno.h"

#ifndef _CLIB_KERNEL_
static BOOL __startup_cd_changed;  /* Need this since NULL is a valid lock */
static BPTR __startup_cd_lock;     
#endif

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
			
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    GETUSER;

    BPTR oldlock;
    BPTR newlock = Lock( path, SHARED_LOCK );

    if( newlock == NULL )
    {
    	errno = IoErr2errno( IoErr() );
	goto error;
    }

    if( SetCurrentDirName( path ) != DOSTRUE ) 
    {
    	errno = ENAMETOOLONG;
	goto error;
    }
    
    oldlock = CurrentDir( newlock );
    
    if( __startup_cd_changed )
    {
    	UnLock( oldlock );
    }
    else
    {
    	__startup_cd_changed = TRUE;
	__startup_cd_lock    = oldlock;
    }    
    
    return 0;

error:
    if( newlock != NULL ) UnLock( newlock );
    
    return -1;
}

int __init_chdir(void)
{
    GETUSER;

    __startup_cd_changed = FALSE;

    return 0;
}

void __exit_chdir(void)
{
    GETUSER;

    if( __startup_cd_changed )
    {
    	BPTR lock = CurrentDir( __startup_cd_lock );
	UnLock( lock );
    }
}

ADD2INIT(__init_chdir, 110);
ADD2EXIT(__exit_chdir, 110);

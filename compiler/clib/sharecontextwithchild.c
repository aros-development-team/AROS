/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$

    AROS function sharecontextwithchild().
*/

#include "__arosc_privdata.h"
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	void sharecontextwithchild (

/*  SYNOPSIS */
	int share) 
        
/*  FUNCTION
        Controls sharing parent arosc context with child processes.
        Must be called by parent process.

    INPUTS
        share - TRUE/FALSE - should parent share context with children

    RESULT

    NOTES
        By calling this function the user of arosc.library controls how
        new child processes use parent's arosc.library context.

        When sharing is enabled, a child process will never create its
        own arosc.library context and will alway use parent's context.

        If you have a parent and child process allocating/dealocating
        (malloc/free) memory for the same pointer, you must use this
        function so that the memory will always be allocated in the
        same arosc.libray context. Failure to do so will result with
        GURU during free.

        Things to be aware off:
        - this function is dangeruos - you as a programmer must take care
        of thread synchronization
        - a child process cannot have life span longer than a parent process
        - a child process may not call vfork or exec* functions - if such
        feature is required, the vfork and exec* functions need to be 
        modified to work with flag set by this function
        - once enabled, the sharing can be disabled at any time - when 
        the sharing is disabled, all child processes will acquire their 
        own arosc contexts when such need arises.

    EXAMPLE

    BUGS

    SEE ALSO
	
    INTERNALS

******************************************************************************/
{
    struct aroscbase *aroscbase = __GM_GetBase();

    if (aroscbase)
    {
        if (share)
            aroscbase->acb_flags |= SHARE_ACPD_WITH_CHILD;
        else
            aroscbase->acb_flags &= ~SHARE_ACPD_WITH_CHILD;
    }
} /* sharecontextwithchild() */


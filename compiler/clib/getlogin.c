/*
    Copyright � 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <errno.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	char * getlogin(

/*  SYNOPSIS */
	)
        
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
    /* Implement getlogin() */
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    
    errno = ENOSYS;
    return NULL;
}

/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <errno.h>

/*****************************************************************************

    NAME */
#include <unistd.h>

	char * ttyname(

/*  SYNOPSIS */
	int fd)
        
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
#   warning Implement ttyname()
    AROS_FUNCTION_NOT_IMPLEMENTED("arosc");
    
    errno = ENOSYS;
    return NULL;
}

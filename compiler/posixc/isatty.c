/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

#include "__fdesc.h"

/*****************************************************************************

    NAME */

#include <unistd.h>

	int isatty(

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
    fdesc *desc = __getfdesc(fd);

    if (desc)
        return IsInteractive(desc->fcb->handle)?1:0;

    return 0;
}


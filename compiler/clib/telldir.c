/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "__dirdesc.h"

/*****************************************************************************

    NAME */

#include <dirent.h>

	long telldir(

/*  SYNOPSIS */
	DIR *dir)

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
    return dir->pos;
}


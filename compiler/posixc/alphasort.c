/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 function alphasort().
*/

#include <string.h>

/*****************************************************************************

    NAME */
#include <dirent.h>

        int __posixc_alphasort (

/*  SYNOPSIS */
        const struct dirent **a,
        const struct dirent **b
        )

/*  FUNCTION
        Support function for scandir().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        __posixc_scandir()

    INTERNALS

******************************************************************************/
{
    return strcoll((*a)->d_name, (*b)->d_name);
}

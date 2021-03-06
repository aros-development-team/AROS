/*
    Copyright (C) 2004, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */
#include <grp.h>

        void endgrent(

/*  SYNOPSIS */
        void)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    /* TODO: Implement endgrent() */
    AROS_FUNCTION_NOT_IMPLEMENTED("posixc");
    errno = ENOSYS;

    return;
}
